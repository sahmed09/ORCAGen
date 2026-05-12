// AntiClipboardLoggerHook.cpp - Enhanced with Registry Deception
#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <easyhook.h>
#include <random>

#pragma comment(lib, "EasyHook32.lib") // or EasyHook64.lib depending on platform

using namespace std;

DWORD gFreqOffset = 0;

// Random generator for deception
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dist(0, 99);

// ============================================================================
// REGISTRY API HOOKS - FakeFailure Strategy
// ============================================================================

// Original function pointers (for calling real APIs)
typedef LONG(WINAPI* RegOpenKeyExW_t)(HKEY, LPCWSTR, DWORD, REGSAM, PHKEY);
typedef LONG(WINAPI* RegQueryValueExW_t)(HKEY, LPCWSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);

RegOpenKeyExW_t pOriginalRegOpenKeyExW = nullptr;
RegQueryValueExW_t pOriginalRegQueryValueExW = nullptr;

// Helper: Check if path is sensitive (Winlogon credential paths)
bool IsSensitiveRegistryPath(LPCWSTR subKey) {
    if (subKey == nullptr) return false;
    
    wstring key(subKey);
    // Convert to lowercase for case-insensitive comparison
    transform(key.begin(), key.end(), key.begin(), ::towlower);
    
    // Detect Winlogon path which contains autologon credentials
    if (key.find(L"winlogon") != wstring::npos) {
        wcout << L"[Deception] Detected sensitive registry path: " << subKey << endl;
        return true;
    }
    
    return false;
}

// Helper: Check if value name is sensitive (credential fields)
bool IsSensitiveValueName(LPCWSTR valueName) {
    if (valueName == nullptr) return false;
    
    wstring valName(valueName);
    transform(valName.begin(), valName.end(), valName.begin(), ::towlower);
    
    // Credential-related value names
    if (valName.find(L"password") != wstring::npos ||
        valName.find(L"username") != wstring::npos ||
        valName.find(L"defaultuser") != wstring::npos ||
        valName.find(L"defaultdomain") != wstring::npos ||
        valName.find(L"autoadminlogon") != wstring::npos ||
        valName.find(L"altdefault") != wstring::npos) {
        wcout << L"[Deception] Detected sensitive value query: " << valueName << endl;
        return true;
    }
    
    return false;
}

// Hooked RegOpenKeyExW - Intercept registry key opening
LONG WINAPI myRegOpenKeyExWHook(
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD ulOptions,
    REGSAM samDesired,
    PHKEY phkResult)
{
    // Check if this is a sensitive path
    bool isSensitive = IsSensitiveRegistryPath(lpSubKey);
    
    if (isSensitive) {
        wcout << L"[Hook] RegOpenKeyExW intercepted - SENSITIVE PATH" << endl;
        wcout << L"    SubKey: " << (lpSubKey ? lpSubKey : L"(null)") << endl;
        
        // FakeFailure Strategy: Return access denied error
        wcout << L"[Deception] Returning ERROR_ACCESS_DENIED to block registry mining" << endl;
        return ERROR_ACCESS_DENIED;
    }
    
    // Allow legitimate registry operations to proceed normally
    return pOriginalRegOpenKeyExW(hKey, lpSubKey, ulOptions, samDesired, phkResult);
}

// Hooked RegQueryValueExW - Intercept registry value queries
LONG WINAPI myRegQueryValueExWHook(
    HKEY hKey,
    LPCWSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData)
{
    // Check if querying sensitive values
    bool isSensitive = IsSensitiveValueName(lpValueName);
    
    if (isSensitive) {
        wcout << L"[Hook] RegQueryValueExW intercepted - SENSITIVE VALUE" << endl;
        wcout << L"    ValueName: " << (lpValueName ? lpValueName : L"(null)") << endl;
        
        // FakeFailure Strategy: Return "value not found" error
        wcout << L"[Deception] Returning ERROR_FILE_NOT_FOUND to hide credential data" << endl;
        return ERROR_FILE_NOT_FOUND;
    }
    
    // Allow legitimate queries to proceed normally
    return pOriginalRegQueryValueExW(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
}

// ============================================================================
// ORIGINAL BEEP HOOK (Preserved from original code)
// ============================================================================

BOOL WINAPI myBeepHook(DWORD dwFreq, DWORD dwDuration) {
    cout << "[+] BeepHook triggered!" << endl;
    cout << "    Original Frequency: " << dwFreq << ", Duration: " << dwDuration << endl;
    return Beep(dwFreq + gFreqOffset, dwDuration);
}

// ============================================================================
// DLL ENTRY POINT
// ============================================================================

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO* inRemoteInfo)
{
    cout << "[*] Anti-Registry Mining Hook - Injection started." << endl;
    
    // Handle user data if provided
    if (inRemoteInfo->UserDataSize == sizeof(DWORD)) {
        gFreqOffset = *reinterpret_cast<DWORD*>(inRemoteInfo->UserData);
        cout << "    Frequency Offset Received: " << gFreqOffset << endl;
    }
    
    // ========================================================================
    // INSTALL REGISTRY HOOKS
    // ========================================================================
    
    HOOK_TRACE_INFO hRegOpenKeyHook = { NULL };
    HOOK_TRACE_INFO hRegQueryValueHook = { NULL };
    
    FARPROC regOpenKeyAddr = GetProcAddress(GetModuleHandle(TEXT("advapi32")), "RegOpenKeyExW");
    FARPROC regQueryValueAddr = GetProcAddress(GetModuleHandle(TEXT("advapi32")), "RegQueryValueExW");
    
    if (regOpenKeyAddr == nullptr || regQueryValueAddr == nullptr) {
        wcerr << L"[ERROR] Failed to locate registry API addresses" << endl;
        return;
    }
    
    cout << "    RegOpenKeyExW address: " << (void*)regOpenKeyAddr << endl;
    cout << "    RegQueryValueExW address: " << (void*)regQueryValueAddr << endl;
    
    // Store original function pointers for legitimate operations
    pOriginalRegOpenKeyExW = (RegOpenKeyExW_t)regOpenKeyAddr;
    pOriginalRegQueryValueExW = (RegQueryValueExW_t)regQueryValueAddr;
    
    // Install RegOpenKeyExW hook
    NTSTATUS result = LhInstallHook(regOpenKeyAddr, myRegOpenKeyExWHook, nullptr, &hRegOpenKeyHook);
    if (FAILED(result)) {
        wstring s(RtlGetLastErrorString());
        wcerr << L"[ERROR] Failed to install RegOpenKeyExW hook: " << s << endl;
    } else {
        cout << "[+] RegOpenKeyExW hook installed successfully!" << endl;
    }
    
    // Install RegQueryValueExW hook
    result = LhInstallHook(regQueryValueAddr, myRegQueryValueExWHook, nullptr, &hRegQueryValueHook);
    if (FAILED(result)) {
        wstring s(RtlGetLastErrorString());
        wcerr << L"[ERROR] Failed to install RegQueryValueExW hook: " << s << endl;
    } else {
        cout << "[+] RegQueryValueExW hook installed successfully!" << endl;
    }
    
    // ========================================================================
    // INSTALL BEEP HOOK (Original functionality)
    // ========================================================================
    
    HOOK_TRACE_INFO hBeepHook = { NULL };
    FARPROC beepAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "Beep");
    
    if (beepAddr) {
        cout << "    Beep function address: " << (void*)beepAddr << endl;
        result = LhInstallHook(beepAddr, myBeepHook, nullptr, &hBeepHook);
        
        if (FAILED(result)) {
            wstring s(RtlGetLastErrorString());
            wcerr << L"[ERROR] Failed to install Beep hook: " << s << endl;
        } else {
            cout << "[+] Beep hook installed successfully!" << endl;
        }
    }
    
    // ========================================================================
    // ACTIVATE ALL HOOKS (Apply to all threads)
    // ========================================================================
    
    ULONG ACLEntries[1] = { 0 };
    LhSetExclusiveACL(ACLEntries, 1, &hRegOpenKeyHook);
    LhSetExclusiveACL(ACLEntries, 1, &hRegQueryValueHook);
    LhSetExclusiveACL(ACLEntries, 1, &hBeepHook);
    
    cout << "[*] All hooks activated. Registry mining protection is active." << endl;
}