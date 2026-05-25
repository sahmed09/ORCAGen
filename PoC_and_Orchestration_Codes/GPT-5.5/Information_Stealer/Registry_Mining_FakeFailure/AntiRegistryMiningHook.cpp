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

static mutex g_lock;
static unordered_set<HKEY> g_blockedRegistryHandles;

bool IsWinlogonPath(HKEY rootKey, LPCWSTR subKey)
{
    if (rootKey != HKEY_LOCAL_MACHINE || subKey == nullptr)
        return false;

    wstring path(subKey);
    transform(path.begin(), path.end(), path.begin(), towlower);

    return path.find(
        L"software\\microsoft\\windows nt\\currentversion\\winlogon"
    ) != wstring::npos;
}

bool IsSensitiveValue(LPCWSTR valueName)
{
    if (valueName == nullptr)
        return false;

    wstring name(valueName);
    transform(name.begin(), name.end(), name.begin(), towlower);

    return name == L"defaultusername" ||
        name == L"defaultpassword" ||
        name == L"defaultdomainname" ||
        name == L"altdefaultusername" ||
        name == L"altdefaultdomainname" ||
        name == L"autoadminlogon";
}

LONG WINAPI myRegOpenKeyExWHook(
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD ulOptions,
    REGSAM samDesired,
    PHKEY phkResult)
{
    if (IsWinlogonPath(hKey, lpSubKey))
    {
        wcout << L"[FakeFailure] Blocked RegOpenKeyExW on: "
            << lpSubKey << endl;

        if (phkResult)
            *phkResult = nullptr;

        SetLastError(ERROR_ACCESS_DENIED);
        return ERROR_ACCESS_DENIED;
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
    bool tracked = false;

    {
        lock_guard<mutex> guard(g_lock);
        tracked = g_blockedRegistryHandles.find(hKey) != g_blockedRegistryHandles.end();
    }

    if (tracked || IsSensitiveValue(lpValueName))
    {
        wcout << L"[FakeFailure] Blocked RegQueryValueExW for value: "
            << (lpValueName ? lpValueName : L"<null>") << endl;

        if (lpType)
            *lpType = REG_NONE;

        if (lpcbData)
            *lpcbData = 0;

        SetLastError(ERROR_ACCESS_DENIED);
        return ERROR_ACCESS_DENIED;
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

LONG WINAPI myRegSetValueExWHook(
    HKEY hKey,
    LPCWSTR lpValueName,
    DWORD Reserved,
    DWORD dwType,
    const BYTE* lpData,
    DWORD cbData)
{
    if (IsSensitiveValue(lpValueName))
    {
        wcout << L"[FakeFailure] Blocked RegSetValueExW for value: "
            << lpValueName << endl;

        SetLastError(ERROR_ACCESS_DENIED);
        return ERROR_ACCESS_DENIED;
    }

    return RegSetValueExW(
        hKey,
        lpValueName,
        Reserved,
        dwType,
        lpData,
        cbData
    );
}

LONG WINAPI myRegCreateKeyExWHook(
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD Reserved,
    LPWSTR lpClass,
    DWORD dwOptions,
    REGSAM samDesired,
    const LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY phkResult,
    LPDWORD lpdwDisposition)
{
    if (IsWinlogonPath(hKey, lpSubKey))
    {
        wcout << L"[FakeFailure] Blocked RegCreateKeyExW on: "
            << lpSubKey << endl;

        if (phkResult)
            *phkResult = nullptr;

        if (lpdwDisposition)
            *lpdwDisposition = 0;

        SetLastError(ERROR_ACCESS_DENIED);
        return ERROR_ACCESS_DENIED;
    }

    return RegCreateKeyExW(
        hKey,
        lpSubKey,
        Reserved,
        lpClass,
        dwOptions,
        samDesired,
        lpSecurityAttributes,
        phkResult,
        lpdwDisposition
    );
}

LONG WINAPI myRegCloseKeyHook(HKEY hKey)
{
    {
        lock_guard<mutex> guard(g_lock);
        g_blockedRegistryHandles.erase(hKey);
    }

    return RegCloseKey(hKey);
}

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(
    REMOTE_ENTRY_INFO * inRemoteInfo)
{
    cout << "[*] Registry FakeFailure DLL injected." << endl;

    HMODULE advapi32 = GetModuleHandleW(L"advapi32.dll");
    if (!advapi32)
        advapi32 = LoadLibraryW(L"advapi32.dll");

    if (!advapi32)
    {
        cerr << "[-] Failed to load advapi32.dll." << endl;
        return;
    }

    HOOK_TRACE_INFO hRegOpenHook = { NULL };
    HOOK_TRACE_INFO hRegQueryHook = { NULL };
    HOOK_TRACE_INFO hRegSetHook = { NULL };
    HOOK_TRACE_INFO hRegCreateHook = { NULL };
    HOOK_TRACE_INFO hRegCloseHook = { NULL };

    LhInstallHook(GetProcAddress(advapi32, "RegOpenKeyExW"),
        myRegOpenKeyExWHook, nullptr, &hRegOpenHook);

    LhInstallHook(GetProcAddress(advapi32, "RegQueryValueExW"),
        myRegQueryValueExWHook, nullptr, &hRegQueryHook);

    LhInstallHook(GetProcAddress(advapi32, "RegSetValueExW"),
        myRegSetValueExWHook, nullptr, &hRegSetHook);

    LhInstallHook(GetProcAddress(advapi32, "RegCreateKeyExW"),
        myRegCreateKeyExWHook, nullptr, &hRegCreateHook);

    LhInstallHook(GetProcAddress(advapi32, "RegCloseKey"),
        myRegCloseKeyHook, nullptr, &hRegCloseHook);

    ULONG ACLEntries[1] = { 0 };

    LhSetExclusiveACL(ACLEntries, 1, &hRegOpenHook);
    LhSetExclusiveACL(ACLEntries, 1, &hRegQueryHook);
    LhSetExclusiveACL(ACLEntries, 1, &hRegSetHook);
    LhSetExclusiveACL(ACLEntries, 1, &hRegCreateHook);
    LhSetExclusiveACL(ACLEntries, 1, &hRegCloseHook);

    cout << "[+] Registry FakeFailure hooks installed successfully." << endl;
}