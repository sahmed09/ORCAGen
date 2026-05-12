#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <map>
#include <easyhook.h>

#pragma comment(lib, "EasyHook32.lib") // or EasyHook64.lib depending on platform

using namespace std;

// ============================================================================
// DECOY DATA STORAGE
// ============================================================================
// Map to store decoy registry values for targeted keys
std::map<std::wstring, std::wstring> decoyRegistryData = {
    {L"DefaultUserName", L"DecoyUser_HoneyAccount"},
    {L"DefaultPassword", L"Dec0yP@ssw0rd!2024"},
    {L"DefaultDomainName", L"DECEPTION_DOMAIN"},
    {L"AutoAdminLogon", L"1"},
    {L"AltDefaultUserName", L"BackupDecoyUser"},
    {L"AltDefaultDomainName", L"ALT_DECEPTION_DOMAIN"}
};

// Track which registry keys are sensitive (Winlogon in this case)
bool IsSensitiveRegistryKey(HKEY hKey) {
    // In a more sophisticated implementation, you would track the actual
    // key path. For this PoC, we'll use a heuristic approach.
    // You could maintain a map of HKEY handles to their paths.
    return true; // Simplified: assume all queried keys are potentially sensitive
}

// ============================================================================
// ORIGINAL FUNCTION POINTERS
// ============================================================================
typedef LONG(WINAPI* RegOpenKeyExW_t)(
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD ulOptions,
    REGSAM samDesired,
    PHKEY phkResult
);

typedef LONG(WINAPI* RegQueryValueExW_t)(
    HKEY hKey,
    LPCWSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
);

typedef LONG(WINAPI* RegCloseKey_t)(
    HKEY hKey
);

RegOpenKeyExW_t OriginalRegOpenKeyExW = nullptr;
RegQueryValueExW_t OriginalRegQueryValueExW = nullptr;
RegCloseKey_t OriginalRegCloseKey = nullptr;

// Store mapping of opened registry keys to their paths for context-aware hooking
std::map<HKEY, std::wstring> openedKeyPaths;

// ============================================================================
// HOOKED FUNCTIONS
// ============================================================================

LONG WINAPI myRegOpenKeyExWHook(
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD ulOptions,
    REGSAM samDesired,
    PHKEY phkResult)
{
    wcout << L"[Hook] RegOpenKeyExW intercepted!" << endl;
    wcout << L"  SubKey: " << (lpSubKey ? lpSubKey : L"(null)") << endl;

    // Call the original function
    LONG result = RegOpenKeyExW(hKey, lpSubKey, ulOptions, samDesired, phkResult);

    // Track opened keys for context-aware value interception
    if (result == ERROR_SUCCESS && phkResult && lpSubKey) {
        std::wstring subKeyStr(lpSubKey);
        // Check if this is the Winlogon key we want to deceive
        if (subKeyStr.find(L"Winlogon") != std::wstring::npos) {
            openedKeyPaths[*phkResult] = subKeyStr;
            wcout << L"  [Deception] Tracked sensitive key: " << subKeyStr << endl;
        }
    }

    return result;
}

LONG WINAPI myRegQueryValueExWHook(
    HKEY hKey,
    LPCWSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData)
{
    bool isDeceptive = false;
    std::wstring valueName = lpValueName ? lpValueName : L"";

    // Check if this key is being tracked (i.e., it's a sensitive key)
    if (openedKeyPaths.find(hKey) != openedKeyPaths.end()) {
        // Check if the requested value is in our decoy data
        if (decoyRegistryData.find(valueName) != decoyRegistryData.end()) {
            wcout << L"[Deception] RegQueryValueExW intercepted for: " << valueName << endl;
            
            std::wstring decoyValue = decoyRegistryData[valueName];
            
            // Calculate required buffer size
            DWORD requiredSize = (decoyValue.length() + 1) * sizeof(wchar_t);
            
            // If lpData is NULL, caller is querying the size
            if (lpData == nullptr) {
                if (lpcbData) {
                    *lpcbData = requiredSize;
                }
                if (lpType) {
                    *lpType = REG_SZ;
                }
                wcout << L"  [Deception] Returned decoy size: " << requiredSize << endl;
                return ERROR_SUCCESS;
            }
            
            // If buffer is provided and large enough, copy decoy data
            if (lpcbData && *lpcbData >= requiredSize) {
                memcpy(lpData, decoyValue.c_str(), requiredSize);
                *lpcbData = requiredSize;
                if (lpType) {
                    *lpType = REG_SZ;
                }
                wcout << L"  [Deception] Returned decoy value: " << decoyValue << endl;
                return ERROR_SUCCESS;
            }
            
            // Buffer too small
            if (lpcbData) {
                *lpcbData = requiredSize;
            }
            return ERROR_MORE_DATA;
        }
    }

    // For non-sensitive keys or values, call the original function
    return RegQueryValueExW(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
}

LONG WINAPI myRegCloseKeyHook(HKEY hKey)
{
    // Clean up our tracking when keys are closed
    if (openedKeyPaths.find(hKey) != openedKeyPaths.end()) {
        wcout << L"[Hook] RegCloseKey: Removing tracked key" << endl;
        openedKeyPaths.erase(hKey);
    }

    return RegCloseKey(hKey);
}

// ============================================================================
// ENTRY POINT
// ============================================================================
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
    cout << "[*] Anti-Registry Mining Injection started." << endl;

    HOOK_TRACE_INFO hRegOpenKeyHook = { NULL };
    HOOK_TRACE_INFO hRegQueryValueHook = { NULL };
    HOOK_TRACE_INFO hRegCloseKeyHook = { NULL };

    // Get addresses of registry functions from advapi32.dll
    HMODULE hAdvapi32 = GetModuleHandle(TEXT("advapi32"));
    if (!hAdvapi32) {
        cerr << "[Error] Failed to get advapi32.dll handle" << endl;
        return;
    }

    FARPROC regOpenKeyAddr = GetProcAddress(hAdvapi32, "RegOpenKeyExW");
    FARPROC regQueryValueAddr = GetProcAddress(hAdvapi32, "RegQueryValueExW");
    FARPROC regCloseKeyAddr = GetProcAddress(hAdvapi32, "RegCloseKey");

    if (!regOpenKeyAddr || !regQueryValueAddr || !regCloseKeyAddr) {
        cerr << "[Error] Failed to get registry function addresses" << endl;
        return;
    }

    cout << "[*] Installing hooks..." << endl;

    // Install RegOpenKeyExW hook
    NTSTATUS result = LhInstallHook(regOpenKeyAddr, myRegOpenKeyExWHook, nullptr, &hRegOpenKeyHook);
    if (FAILED(result)) {
        wstring s(RtlGetLastErrorString());
        wcerr << L"[Error] Failed to install RegOpenKeyExW hook: " << s << endl;
    } else {
        cout << "[+] RegOpenKeyExW hook installed successfully!" << endl;
    }

    // Install RegQueryValueExW hook
    result = LhInstallHook(regQueryValueAddr, myRegQueryValueExWHook, nullptr, &hRegQueryValueHook);
    if (FAILED(result)) {
        wstring s(RtlGetLastErrorString());
        wcerr << L"[Error] Failed to install RegQueryValueExW hook: " << s << endl;
    } else {
        cout << "[+] RegQueryValueExW hook installed successfully!" << endl;
    }

    // Install RegCloseKey hook
    result = LhInstallHook(regCloseKeyAddr, myRegCloseKeyHook, nullptr, &hRegCloseKeyHook);
    if (FAILED(result)) {
        wstring s(RtlGetLastErrorString());
        wcerr << L"[Error] Failed to install RegCloseKey hook: " << s << endl;
    } else {
        cout << "[+] RegCloseKey hook installed successfully!" << endl;
    }

    // Enable hooks for all threads
    ULONG ACLEntries[1] = { 0 };
    LhSetExclusiveACL(ACLEntries, 1, &hRegOpenKeyHook);
    LhSetExclusiveACL(ACLEntries, 1, &hRegQueryValueHook);
    LhSetExclusiveACL(ACLEntries, 1, &hRegCloseKeyHook);

    cout << "[*] All hooks activated. Deception layer active." << endl;
}