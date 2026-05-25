#include "pch.h"
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winreg.h>
#include <iostream>
#include <string>
#include <unordered_set>
#include <mutex>
#include <algorithm>
#include <easyhook.h>

#pragma comment(lib, "EasyHook32.lib") // Use EasyHook64.lib for x64

using namespace std;

static mutex g_mutex;
static unordered_set<HKEY> g_fakeSuccessRegistryHandles;

bool IsTargetWinlogonKey(HKEY rootKey, LPCWSTR subKey)
{
    if (rootKey != HKEY_LOCAL_MACHINE || subKey == nullptr)
        return false;

    wstring keyPath(subKey);
    transform(keyPath.begin(), keyPath.end(), keyPath.begin(), towlower);

    return keyPath.find(
        L"software\\microsoft\\windows nt\\currentversion\\winlogon"
    ) != wstring::npos;
}

bool IsSensitiveWinlogonValue(LPCWSTR valueName)
{
    if (valueName == nullptr)
        return false;

    wstring name(valueName);
    transform(name.begin(), name.end(), name.begin(), towlower);

    return name == L"defaultusername" ||
        name == L"defaultpassword" ||
        name == L"altdefaultusername" ||
        name == L"defaultdomainname" ||
        name == L"autoadminlogon";
}

LONG WINAPI myRegOpenKeyExWHook(
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD ulOptions,
    REGSAM samDesired,
    PHKEY phkResult)
{
    if (IsTargetWinlogonKey(hKey, lpSubKey))
    {
        LONG result = RegOpenKeyExW(
            hKey,
            lpSubKey,
            ulOptions,
            samDesired,
            phkResult
        );

        if (result == ERROR_SUCCESS && phkResult && *phkResult)
        {
            lock_guard<mutex> lock(g_mutex);
            g_fakeSuccessRegistryHandles.insert(*phkResult);

            wcout << L"[FakeSuccess] Target Winlogon key opened and handle tagged: "
                << lpSubKey << endl;
        }

        return result;
    }

    return RegOpenKeyExW(
        hKey,
        lpSubKey,
        ulOptions,
        samDesired,
        phkResult
    );
}

LONG WINAPI myRegQueryValueExWHook(
    HKEY hKey,
    LPCWSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData)
{
    bool isTrackedHandle = false;

    {
        lock_guard<mutex> lock(g_mutex);
        isTrackedHandle =
            g_fakeSuccessRegistryHandles.find(hKey) != g_fakeSuccessRegistryHandles.end();
    }

    if (isTrackedHandle && IsSensitiveWinlogonValue(lpValueName))
    {
        if (lpType)
            *lpType = REG_SZ;

        /*
            FakeSuccess strategy:
            - Return ERROR_SUCCESS.
            - Do not query the real registry value.
            - Do not copy real registry data.
            - Return an empty REG_SZ string so the malware believes
              the query completed successfully.
        */

        const wchar_t fakeEmptyValue[] = L"";
        DWORD requiredSize = sizeof(fakeEmptyValue);

        if (lpcbData)
        {
            if (lpData == nullptr || *lpcbData < requiredSize)
            {
                *lpcbData = requiredSize;
                return ERROR_MORE_DATA;
            }

            memcpy(lpData, fakeEmptyValue, requiredSize);
            *lpcbData = requiredSize;
        }

        wcout << L"[FakeSuccess] Suppressed registry mining query: "
            << lpValueName << endl;

        return ERROR_SUCCESS;
    }

    return RegQueryValueExW(
        hKey,
        lpValueName,
        lpReserved,
        lpType,
        lpData,
        lpcbData
    );
}

LONG WINAPI myRegCloseKeyHook(HKEY hKey)
{
    {
        lock_guard<mutex> lock(g_mutex);

        auto it = g_fakeSuccessRegistryHandles.find(hKey);
        if (it != g_fakeSuccessRegistryHandles.end())
        {
            g_fakeSuccessRegistryHandles.erase(it);
            wcout << L"[Cleanup] Removed tracked registry handle." << endl;
        }
    }

    return RegCloseKey(hKey);
}

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(
    REMOTE_ENTRY_INFO * inRemoteInfo)
{
    cout << "[*] Registry FakeSuccess deception DLL injected." << endl;

    HOOK_TRACE_INFO hRegOpenHook = { NULL };
    HOOK_TRACE_INFO hRegQueryHook = { NULL };
    HOOK_TRACE_INFO hRegCloseHook = { NULL };

    HMODULE advapi32 = GetModuleHandleW(L"advapi32.dll");
    if (!advapi32)
        advapi32 = LoadLibraryW(L"advapi32.dll");

    if (!advapi32)
    {
        cerr << "[-] Failed to load advapi32.dll." << endl;
        return;
    }

    FARPROC regOpenAddr = GetProcAddress(advapi32, "RegOpenKeyExW");
    FARPROC regQueryAddr = GetProcAddress(advapi32, "RegQueryValueExW");
    FARPROC regCloseAddr = GetProcAddress(advapi32, "RegCloseKey");

    if (!regOpenAddr || !regQueryAddr || !regCloseAddr)
    {
        cerr << "[-] Failed to resolve registry API addresses." << endl;
        return;
    }

    NTSTATUS result;

    result = LhInstallHook(
        regOpenAddr,
        myRegOpenKeyExWHook,
        nullptr,
        &hRegOpenHook
    );

    if (FAILED(result))
    {
        wcerr << L"[-] Failed to hook RegOpenKeyExW: "
            << RtlGetLastErrorString() << endl;
        return;
    }

    result = LhInstallHook(
        regQueryAddr,
        myRegQueryValueExWHook,
        nullptr,
        &hRegQueryHook
    );

    if (FAILED(result))
    {
        wcerr << L"[-] Failed to hook RegQueryValueExW: "
            << RtlGetLastErrorString() << endl;
        return;
    }

    result = LhInstallHook(
        regCloseAddr,
        myRegCloseKeyHook,
        nullptr,
        &hRegCloseHook
    );

    if (FAILED(result))
    {
        wcerr << L"[-] Failed to hook RegCloseKey: "
            << RtlGetLastErrorString() << endl;
        return;
    }

    ULONG ACLEntries[1] = { 0 };

    LhSetExclusiveACL(ACLEntries, 1, &hRegOpenHook);
    LhSetExclusiveACL(ACLEntries, 1, &hRegQueryHook);
    LhSetExclusiveACL(ACLEntries, 1, &hRegCloseHook);

    cout << "[+] Registry FakeSuccess hooks installed successfully." << endl;
}