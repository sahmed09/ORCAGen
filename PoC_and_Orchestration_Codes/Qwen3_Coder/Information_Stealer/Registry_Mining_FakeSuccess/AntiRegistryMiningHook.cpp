#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <easyhook.h>
#include <vector>
#include <random>
#include <psapi.h>

#pragma comment(lib, "psapi.lib")  // Add this for EnumProcessModules and GetModuleBaseNameA
#pragma comment(lib, "EasyHook32.lib")

using namespace std;

// Random generator for deception
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dist(0, 99);

// Store original function pointers
typedef LONG(WINAPI* RegOpenKeyExA_t)(HKEY, LPCSTR, DWORD, REGSAM, PHKEY);
typedef LONG(WINAPI* RegQueryValueExA_t)(HKEY, LPCSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);

RegOpenKeyExA_t OriginalRegOpenKeyExA = nullptr;
RegQueryValueExA_t OriginalRegQueryValueExA = nullptr;

// Fake registry values for deception
const char* FAKE_DEFAULT_USERNAME = "FAKE_USER";
const char* FAKE_DEFAULT_PASSWORD = "FAKE_PASSWORD";
const char* FAKE_AUTO_ADMIN_LOGON = "FAKE";

// Check if the calling process is the malware (by process name or other detection)
bool IsMalwareProcess()
{
    char szProcessName[MAX_PATH] = { 0 };
    HMODULE hMod;
    DWORD cbNeeded;

    if (EnumProcessModules(GetCurrentProcess(), &hMod, sizeof(hMod), &cbNeeded))
    {
        GetModuleBaseNameA(GetCurrentProcess(), hMod, szProcessName, sizeof(szProcessName));
        // If process name matches known malware pattern
        if (strstr(szProcessName, "malware") || strstr(szProcessName, "ClipboardLoggerPOC"))
            return true;
    }
    return false;
}

// Hook for RegOpenKeyExA
LONG WINAPI myRegOpenKeyExAHook(HKEY hKey, LPCSTR lpSubKey, DWORD dwReserved, REGSAM samDesired, PHKEY phkResult)
{
    // If this is the malware trying to access Winlogon registry key
    if (IsMalwareProcess() && lpSubKey && strstr(lpSubKey, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"))
    {
        // Return success but don't actually open the key
        *phkResult = hKey;  // Return the original handle or a fake one
        return ERROR_SUCCESS;
    }

    // Call original function for legitimate applications
    return OriginalRegOpenKeyExA(hKey, lpSubKey, dwReserved, samDesired, phkResult);
}

// Hook for RegQueryValueExA
LONG WINAPI myRegQueryValueExAHook(HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
    // If this is the malware trying to access sensitive values
    if (IsMalwareProcess() && lpValueName)
    {
        // Return fake data for DefaultUserName, DefaultPassword, AutoAdminLogon
        if (strcmp(lpValueName, "DefaultUserName") == 0)
        {
            if (lpData && lpcbData)
            {
                strcpy_s((char*)lpData, *lpcbData, FAKE_DEFAULT_USERNAME);
                *lpcbData = strlen(FAKE_DEFAULT_USERNAME) + 1;
                return ERROR_SUCCESS;
            }
        }
        else if (strcmp(lpValueName, "DefaultPassword") == 0)
        {
            if (lpData && lpcbData)
            {
                strcpy_s((char*)lpData, *lpcbData, FAKE_DEFAULT_PASSWORD);
                *lpcbData = strlen(FAKE_DEFAULT_PASSWORD) + 1;
                return ERROR_SUCCESS;
            }
        }
        else if (strcmp(lpValueName, "AutoAdminLogon") == 0)
        {
            if (lpData && lpcbData)
            {
                strcpy_s((char*)lpData, *lpcbData, FAKE_AUTO_ADMIN_LOGON);
                *lpcbData = strlen(FAKE_AUTO_ADMIN_LOGON) + 1;
                return ERROR_SUCCESS;
            }
        }
    }

    // Call original function for legitimate applications
    return OriginalRegQueryValueExA(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
}

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
    cout << "[*] Registry Hooking DLL Injection started." << endl;

    // Get addresses of original functions
    FARPROC regOpenKeyAddr = GetProcAddress(GetModuleHandleA("advapi32"), "RegOpenKeyExA");
    FARPROC regQueryValueAddr = GetProcAddress(GetModuleHandleA("advapi32"), "RegQueryValueExA");

    if (!regOpenKeyAddr || !regQueryValueAddr)
    {
        cout << "[!] Failed to get function addresses." << endl;
        return;
    }

    // Install hooks
    HOOK_TRACE_INFO hRegOpenHook = { NULL };
    HOOK_TRACE_INFO hRegQueryHook = { NULL };

    LhInstallHook(regOpenKeyAddr, myRegOpenKeyExAHook, nullptr, &hRegOpenHook);
    LhInstallHook(regQueryValueAddr, myRegQueryValueExAHook, nullptr, &hRegQueryHook);

    // Enable all hooks
    ULONG ACLEntries[1] = { 0 };
    LhSetExclusiveACL(ACLEntries, 1, &hRegOpenHook);
    LhSetExclusiveACL(ACLEntries, 1, &hRegQueryHook);

    // Store original function pointers for future use
    OriginalRegOpenKeyExA = (RegOpenKeyExA_t)regOpenKeyAddr;
    OriginalRegQueryValueExA = (RegQueryValueExA_t)regQueryValueAddr;

    cout << "[*] Registry hooks installed successfully." << endl;
}