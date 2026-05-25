#include "pch.h"
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <algorithm>
#include <easyhook.h>

#pragma comment(lib, "EasyHook32.lib") // Use EasyHook64.lib for x64 builds

using namespace std;

static mutex g_mutex;
static unordered_set<HKEY> g_deceptiveKeys;

static unordered_map<wstring, wstring> g_decoyValues = {
    { L"DefaultUserName",      L"decoy-admin" },
    { L"DefaultPassword",      L"P@ssw0rd-Decoy-2026!" },
    { L"DefaultDomainName",    L"DECOY-DOMAIN" },
    { L"AutoAdminLogon",       L"1" },
    { L"AltDefaultUserName",   L"backup-decoy-user" },
    { L"AltDefaultDomainName", L"DECOY-BACKUP-DOMAIN" }
};

bool ContainsWinlogonPath(const wstring& path)
{
    wstring lower = path;
    transform(lower.begin(), lower.end(), lower.begin(), towlower);

    return lower.find(L"software\\microsoft\\windows nt\\currentversion\\winlogon") != wstring::npos;
}

LONG WINAPI myRegOpenKeyExWHook(
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD ulOptions,
    REGSAM samDesired,
    PHKEY phkResult)
{
    LONG ret = RegOpenKeyExW(
        hKey,
        lpSubKey,
        ulOptions,
        samDesired,
        phkResult
    );

    if (ret == ERROR_SUCCESS && lpSubKey && phkResult && *phkResult)
    {
        wstring subKey(lpSubKey);

        if (hKey == HKEY_LOCAL_MACHINE && ContainsWinlogonPath(subKey))
        {
            lock_guard<mutex> lock(g_mutex);
            g_deceptiveKeys.insert(*phkResult);

            wcout << L"[Deception] Winlogon registry key opened. Handle tagged: "
                << *phkResult << endl;
        }
        else
        {
            wcout << L"[Registry] Normal RegOpenKeyExW: " << subKey << endl;
        }
    }

    return ret;
}

LONG WINAPI myRegQueryValueExWHook(
    HKEY hKey,
    LPCWSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData)
{
    bool isTaggedHandle = false;

    {
        lock_guard<mutex> lock(g_mutex);
        isTaggedHandle = g_deceptiveKeys.find(hKey) != g_deceptiveKeys.end();
    }

    if (isTaggedHandle && lpValueName)
    {
        wstring valueName(lpValueName);

        auto it = g_decoyValues.find(valueName);
        if (it != g_decoyValues.end())
        {
            const wstring& decoy = it->second;
            DWORD requiredSize = static_cast<DWORD>((decoy.size() + 1) * sizeof(wchar_t));

            if (lpType)
                *lpType = REG_SZ;

            if (lpcbData)
            {
                if (!lpData || *lpcbData < requiredSize)
                {
                    *lpcbData = requiredSize;
                    return ERROR_MORE_DATA;
                }

                memcpy(lpData, decoy.c_str(), requiredSize);
                *lpcbData = requiredSize;
            }

            wcout << L"[Deception] RegQueryValueExW redirected: "
                << valueName << L" -> " << decoy << endl;

            return ERROR_SUCCESS;
        }
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
        auto it = g_deceptiveKeys.find(hKey);

        if (it != g_deceptiveKeys.end())
        {
            g_deceptiveKeys.erase(it);
            wcout << L"[Cleanup] Removed deceptive registry handle: "
                << hKey << endl;
        }
    }

    return RegCloseKey(hKey);
}

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(
    REMOTE_ENTRY_INFO * inRemoteInfo)
{
    cout << "[*] Registry deception DLL injected." << endl;

    HOOK_TRACE_INFO hRegOpenHook = { NULL };
    HOOK_TRACE_INFO hRegQueryHook = { NULL };
    HOOK_TRACE_INFO hRegCloseHook = { NULL };

    HMODULE advapi = GetModuleHandleW(L"advapi32.dll");
    if (!advapi)
        advapi = LoadLibraryW(L"advapi32.dll");

    if (!advapi)
    {
        cerr << "[-] Failed to load advapi32.dll" << endl;
        return;
    }

    FARPROC regOpenAddr = GetProcAddress(advapi, "RegOpenKeyExW");
    FARPROC regQueryAddr = GetProcAddress(advapi, "RegQueryValueExW");
    FARPROC regCloseAddr = GetProcAddress(advapi, "RegCloseKey");

    if (!regOpenAddr || !regQueryAddr || !regCloseAddr)
    {
        cerr << "[-] Failed to resolve registry API addresses." << endl;
        return;
    }

    NTSTATUS result;

    result = LhInstallHook(regOpenAddr, myRegOpenKeyExWHook, nullptr, &hRegOpenHook);
    if (FAILED(result))
    {
        wcerr << L"[-] Failed to hook RegOpenKeyExW: "
            << RtlGetLastErrorString() << endl;
        return;
    }

    result = LhInstallHook(regQueryAddr, myRegQueryValueExWHook, nullptr, &hRegQueryHook);
    if (FAILED(result))
    {
        wcerr << L"[-] Failed to hook RegQueryValueExW: "
            << RtlGetLastErrorString() << endl;
        return;
    }

    result = LhInstallHook(regCloseAddr, myRegCloseKeyHook, nullptr, &hRegCloseHook);
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

    cout << "[+] Registry deception hooks installed successfully." << endl;
}