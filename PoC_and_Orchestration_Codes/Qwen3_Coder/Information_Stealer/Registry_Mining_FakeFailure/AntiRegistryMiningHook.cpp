#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <algorithm>  // Added this line for transform
#include <easyhook.h>

using namespace std;

// Forward declarations for hooks
LONG WINAPI myRegOpenKeyExA(
    HKEY hKey,
    LPCSTR lpSubKey,
    DWORD dwReserved,
    REGSAM samDesired,
    PHKEY phkResult
);

LONG WINAPI myRegQueryValueExA(
    HKEY hKey,
    LPCSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
);

// Global flag to track if we are currently in a hooked process (optional)
bool IsMalwareDetected = false;

extern "C" __declspec(dllexport) void __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
    cout << "[*] Registry defense hooking initialized." << endl;

    HOOK_TRACE_INFO hRegOpenHook = { NULL };
    HOOK_TRACE_INFO hRegQueryHook = { NULL };

    // Hook RegOpenKeyExA
    FARPROC regOpenAddr = GetProcAddress(GetModuleHandleA("advapi32"), "RegOpenKeyExA");
    if (regOpenAddr != nullptr)
    {
        LhInstallHook(regOpenAddr, myRegOpenKeyExA, nullptr, &hRegOpenHook);
        cout << "[+] Hooked RegOpenKeyExA" << endl;
    }

    // Hook RegQueryValueExA
    FARPROC regQueryAddr = GetProcAddress(GetModuleHandleA("advapi32"), "RegQueryValueExA");
    if (regQueryAddr != nullptr)
    {
        LhInstallHook(regQueryAddr, myRegQueryValueExA, nullptr, &hRegQueryHook);
        cout << "[+] Hooked RegQueryValueExA" << endl;
    }

    // Enable hooks
    ULONG ACLEntries[1] = { 0 };
    LhSetExclusiveACL(ACLEntries, 1, &hRegOpenHook);
    LhSetExclusiveACL(ACLEntries, 1, &hRegQueryHook);

    cout << "[*] All registry hooks installed." << endl;
}

LONG WINAPI myRegOpenKeyExA(
    HKEY hKey,
    LPCSTR lpSubKey,
    DWORD dwReserved,
    REGSAM samDesired,
    PHKEY phkResult
)
{
    // Forward to original function
    typedef LONG(WINAPI* RegOpenKeyExA_t)(HKEY, LPCSTR, DWORD, REGSAM, PHKEY);
    static RegOpenKeyExA_t RealRegOpenKeyExA = (RegOpenKeyExA_t)GetProcAddress(GetModuleHandleA("advapi32"), "RegOpenKeyExA");

    if (RealRegOpenKeyExA == nullptr)
        return ERROR_ACCESS_DENIED;

    LONG result = RealRegOpenKeyExA(hKey, lpSubKey, dwReserved, samDesired, phkResult);

    // Check for Winlogon key access
    string subKey(lpSubKey);
    transform(subKey.begin(), subKey.end(), subKey.begin(), ::tolower);

    if (subKey.find("software\\microsoft\\windows nt\\currentversion\\winlogon") != string::npos)
    {
        cout << "[!] Malware accessing Winlogon registry key!" << endl;
        IsMalwareDetected = true;
    }

    return result;
}

LONG WINAPI myRegQueryValueExA(
    HKEY hKey,
    LPCSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
)
{
    // Forward to original function
    typedef LONG(WINAPI* RegQueryValueExA_t)(HKEY, LPCSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
    static RegQueryValueExA_t RealRegQueryValueExA = (RegQueryValueExA_t)GetProcAddress(GetModuleHandleA("advapi32"), "RegQueryValueExA");

    if (RealRegQueryValueExA == nullptr)
        return ERROR_ACCESS_DENIED;

    // Track value names
    string valueName(lpValueName);
    transform(valueName.begin(), valueName.end(), valueName.begin(), ::tolower);

    // If the malware is trying to read sensitive data, simulate a failure
    if (valueName == "defaultpassword" || valueName == "defaultusername" || valueName == "autoadminlogon")
    {
        cout << "[!] Malware attempting to retrieve credential: " << valueName << endl;

        // Simulate a registry error for the malware
        return ERROR_FILE_NOT_FOUND;
    }

    // Otherwise, proceed normally
    return RealRegQueryValueExA(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
}