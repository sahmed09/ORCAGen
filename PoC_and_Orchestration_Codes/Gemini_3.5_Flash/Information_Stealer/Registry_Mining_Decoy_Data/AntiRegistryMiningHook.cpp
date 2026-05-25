#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winreg.h>
#include <iostream>
#include <string>
#include <set>
#include <mutex>
#include <easyhook.h>

#pragma comment(lib, "EasyHook32.lib") // Change to EasyHook32.lib if targeting a 32-bit PoC

using namespace std;

// Thread-safe collection to track HKEY handles associated with the Winlogon key
std::set<HKEY> g_WinlogonHandles;
std::mutex g_RegistryMutex;

// Function pointers for invoking the original APIs inside our hooks
typedef LONG(WINAPI* RegOpenKeyExW_pfn)(HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult);
typedef LONG(WINAPI* RegQueryValueExW_pfn)(HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
typedef LONG(WINAPI* RegCloseKey_pfn)(HKEY hKey);

RegOpenKeyExW_pfn Original_RegOpenKeyExW = nullptr;
RegQueryValueExW_pfn Original_RegQueryValueExW = nullptr;
RegCloseKey_pfn Original_RegCloseKey = nullptr;


// 1. Hook for RegOpenKeyExW
LONG WINAPI Detour_RegOpenKeyExW(HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult)
{
    // Call the original function to get a valid system handle layout
    LONG result = Original_RegOpenKeyExW(hKey, lpSubKey, ulOptions, samDesired, phkResult);

    if (result == ERROR_SUCCESS && lpSubKey != nullptr && phkResult != nullptr)
    {
        std::wstring subKeyStr(lpSubKey);
        // Detect if the target application is trying to access the credential vault key
        if (subKeyStr.find(L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon") != std::wstring::npos)
        {
            cout << "[Active Defense] Intercepted RegOpenKeyExW for Winlogon. Tracking handle: " << *phkResult << endl;
            std::lock_guard<std::mutex> lock(g_RegistryMutex);
            g_WinlogonHandles.insert(*phkResult);
        }
    }
    return result;
}

// 2. Hook for RegQueryValueExW
LONG WINAPI Detour_RegQueryValueExW(HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
    bool isTargetHandle = false;
    {
        std::lock_guard<std::mutex> lock(g_RegistryMutex);
        if (g_WinlogonHandles.find(hKey) != g_WinlogonHandles.end())
        {
            isTargetHandle = true;
        }
    }

    // If this handle points to our tracked Winlogon path, execute the deception framework strategy
    if (isTargetHandle && lpValueName != nullptr)
    {
        std::wstring valueName(lpValueName);

        if (valueName == L"DefaultUserName")
        {
            cout << "[Deception] Feeding honey-pot username data." << endl;
            if (lpType) *lpType = REG_SZ;

            LPCWSTR decoyUser = L"honey_admin";
            DWORD requiredSize = (wcslen(decoyUser) + 1) * sizeof(WCHAR);

            if (lpData == nullptr) // Caller is querying for buffer size requirements
            {
                *lpcbData = requiredSize;
                return ERROR_SUCCESS;
            }
            if (*lpcbData < requiredSize)
            {
                *lpcbData = requiredSize;
                return ERROR_MORE_DATA;
            }

            wcscpy_s((WCHAR*)lpData, *lpcbData / sizeof(WCHAR), decoyUser);
            *lpcbData = requiredSize;
            return ERROR_SUCCESS;
        }
        else if (valueName == L"DefaultPassword")
        {
            cout << "[Deception] Feeding honey-pot password data." << endl;
            if (lpType) *lpType = REG_SZ;

            LPCWSTR decoyPass = L"P@ssw0rd_HoneyPot_2026!";
            DWORD requiredSize = (wcslen(decoyPass) + 1) * sizeof(WCHAR);

            if (lpData == nullptr)
            {
                *lpcbData = requiredSize;
                return ERROR_SUCCESS;
            }
            if (*lpcbData < requiredSize)
            {
                *lpcbData = requiredSize;
                return ERROR_MORE_DATA;
            }

            wcscpy_s((WCHAR*)lpData, *lpcbData / sizeof(WCHAR), decoyPass);
            *lpcbData = requiredSize;
            return ERROR_SUCCESS;
        }
        else if (valueName == L"AutoAdminLogon")
        {
            cout << "[Deception] Forcing AutoAdminLogon to present as Enabled." << endl;
            if (lpType) *lpType = REG_DWORD;

            DWORD requiredSize = sizeof(DWORD);
            if (lpData == nullptr)
            {
                *lpcbData = requiredSize;
                return ERROR_SUCCESS;
            }
            if (*lpcbData < requiredSize)
            {
                *lpcbData = requiredSize;
                return ERROR_MORE_DATA;
            }

            // The PoC checks for boolean truth value blocks; provide a hardcoded true indicator
            *(DWORD*)lpData = 1;
            *lpcbData = requiredSize;
            return ERROR_SUCCESS;
        }
    }

    // Fall back completely to normal registry behavior for non-targeted queries
    return Original_RegQueryValueExW(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
}

// 3. Hook for RegCloseKey (Housekeeping handle leaks inside framework memory tracking)
LONG WINAPI Detour_RegCloseKey(HKEY hKey)
{
    {
        std::lock_guard<std::mutex> lock(g_RegistryMutex);
        auto it = g_WinlogonHandles.find(hKey);
        if (it != g_WinlogonHandles.end())
        {
            g_WinlogonHandles.erase(it);
        }
    }
    return Original_RegCloseKey(hKey);
}


// Injection Entry Point called automatically by EasyHook
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
    cout << "[*] Cyber Deception Engine Active inside target process." << endl;

    HOOK_TRACE_INFO hRegOpen = { NULL };
    HOOK_TRACE_INFO hRegQuery = { NULL };
    HOOK_TRACE_INFO hRegClose = { NULL };

    HMODULE hAdvapi32 = GetModuleHandle(TEXT("advapi32.dll"));
    if (!hAdvapi32)
    {
        hAdvapi32 = LoadLibrary(TEXT("advapi32.dll"));
    }

    if (hAdvapi32)
    {
        Original_RegOpenKeyExW = (RegOpenKeyExW_pfn)GetProcAddress(hAdvapi32, "RegOpenKeyExW");
        Original_RegQueryValueExW = (RegQueryValueExW_pfn)GetProcAddress(hAdvapi32, "RegQueryValueExW");
        Original_RegCloseKey = (RegCloseKey_pfn)GetProcAddress(hAdvapi32, "RegCloseKey");

        // Bind detour chains using EasyHook tracking engine
        LhInstallHook((FARPROC)Original_RegOpenKeyExW, Detour_RegOpenKeyExW, nullptr, &hRegOpen);
        LhInstallHook((FARPROC)Original_RegQueryValueExW, Detour_RegQueryValueExW, nullptr, &hRegQuery);
        LhInstallHook((FARPROC)Original_RegCloseKey, Detour_RegCloseKey, nullptr, &hRegClose);

        // Apply global interception permissions across thread configurations within target memory
        ULONG ACLEntries[1] = { 0 };
        LhSetExclusiveACL(ACLEntries, 1, &hRegOpen);
        LhSetExclusiveACL(ACLEntries, 1, &hRegQuery);
        LhSetExclusiveACL(ACLEntries, 1, &hRegClose);

        cout << "[+] Active defense hooks successfully mounted onto Advapi32 APIs." << endl;
    }
    else
    {
        cerr << "[-] Critical Error: Unable to access advapi32.dll reference." << endl;
    }
}