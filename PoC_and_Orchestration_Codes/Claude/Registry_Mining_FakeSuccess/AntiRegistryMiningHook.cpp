#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <map>
#include <algorithm>
#include <easyhook.h>

#pragma comment(lib, "EasyHook32.lib") // or EasyHook64.lib depending on platform

using namespace std;

// Store original function pointers
typedef LONG(WINAPI* RegOpenKeyExW_t)(HKEY, LPCWSTR, DWORD, REGSAM, PHKEY);
typedef LONG(WINAPI* RegQueryValueExW_t)(HKEY, LPCWSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
typedef LONG(WINAPI* RegCloseKey_t)(HKEY);

RegOpenKeyExW_t OriginalRegOpenKeyExW = nullptr;
RegQueryValueExW_t OriginalRegQueryValueExW = nullptr;
RegCloseKey_t OriginalRegCloseKey = nullptr;

// Track suspicious keys that were opened
std::map<HKEY, std::wstring> suspiciousKeys;

// Helper function to convert wide string to lowercase
std::wstring ToLower(const std::wstring& str)
{
    std::wstring result = str;
    for (size_t i = 0; i < result.length(); i++)
    {
        result[i] = towlower(result[i]);
    }
    return result;
}

// Sensitive registry paths to monitor
bool IsSensitiveRegistryPath(LPCWSTR subKey)
{
    if (subKey == nullptr) return false;

    std::wstring key = ToLower(std::wstring(subKey));

    // Check for sensitive registry locations
    return (key.find(L"winlogon") != std::wstring::npos ||
            key.find(L"credentials") != std::wstring::npos ||
            key.find(L"password") != std::wstring::npos ||
            key.find(L"autologon") != std::wstring::npos);
}

// Hook: RegOpenKeyExW
LONG WINAPI myRegOpenKeyExWHook(
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD ulOptions,
    REGSAM samDesired,
    PHKEY phkResult)
{
    wcout << L"[Hook] RegOpenKeyExW intercepted: " << (lpSubKey ? lpSubKey : L"<null>") << endl;

    // Check if this is a sensitive registry path
    if (IsSensitiveRegistryPath(lpSubKey))
    {
        wcout << L"[!] Sensitive registry path detected: " << lpSubKey << endl;
        wcout << L"[Deception] Allowing access but marking for query interception..." << endl;

        // Call original function
        LONG result = OriginalRegOpenKeyExW(hKey, lpSubKey, ulOptions, samDesired, phkResult);

        // Track this key handle for future query interception
        if (result == ERROR_SUCCESS && phkResult != nullptr)
        {
            suspiciousKeys[*phkResult] = lpSubKey;
        }

        return result;
    }

    // For non-sensitive paths, pass through normally
    return OriginalRegOpenKeyExW(hKey, lpSubKey, ulOptions, samDesired, phkResult);
}

// Hook: RegQueryValueExW
LONG WINAPI myRegQueryValueExWHook(
    HKEY hKey,
    LPCWSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData)
{
    // Check if this key handle is being tracked (suspicious)
    std::map<HKEY, std::wstring>::iterator it = suspiciousKeys.find(hKey);
    if (it != suspiciousKeys.end())
    {
        wcout << L"[Hook] RegQueryValueExW intercepted on suspicious key: " 
              << it->second << L"\\" << (lpValueName ? lpValueName : L"<default>") << endl;

        // Check for sensitive value names
        if (lpValueName != nullptr)
        {
            std::wstring valueName = ToLower(std::wstring(lpValueName));

            if (valueName.find(L"password") != std::wstring::npos ||
                valueName.find(L"username") != std::wstring::npos ||
                valueName.find(L"autoadminlogon") != std::wstring::npos ||
                valueName.find(L"domain") != std::wstring::npos)
            {
                wcout << L"[!] Sensitive value query detected: " << lpValueName << endl;
                wcout << L"[Deception] Applying FakeSuccess strategy..." << endl;

                // Generate fake data based on value name
                std::wstring fakeData;
                if (valueName.find(L"password") != std::wstring::npos)
                {
                    fakeData = L"HoneyP0t_FakeP@ss123";
                }
                else if (valueName.find(L"username") != std::wstring::npos)
                {
                    fakeData = L"DecoyUser_HoneyTrap";
                }
                else if (valueName.find(L"domain") != std::wstring::npos)
                {
                    fakeData = L"DECEPTION_DOMAIN";
                }
                else if (valueName.find(L"autoadminlogon") != std::wstring::npos)
                {
                    fakeData = L"1";
                }

                // Return fake data if buffer is provided
                if (lpData != nullptr && lpcbData != nullptr)
                {
                    DWORD requiredSize = static_cast<DWORD>((fakeData.length() + 1) * sizeof(WCHAR));

                    if (*lpcbData >= requiredSize)
                    {
                        wcscpy_s(reinterpret_cast<WCHAR*>(lpData), *lpcbData / sizeof(WCHAR), fakeData.c_str());
                        *lpcbData = requiredSize;

                        if (lpType != nullptr)
                            *lpType = REG_SZ;

                        wcout << L"[Deception] Returned fake data: " << fakeData << endl;
                        return ERROR_SUCCESS; // FakeSuccess
                    }
                    else
                    {
                        *lpcbData = requiredSize;
                        return ERROR_MORE_DATA;
                    }
                }
                else if (lpcbData != nullptr)
                {
                    // Query for size only
                    *lpcbData = static_cast<DWORD>((fakeData.length() + 1) * sizeof(WCHAR));
                    if (lpType != nullptr)
                        *lpType = REG_SZ;
                    return ERROR_SUCCESS;
                }
            }
        }
    }

    // For non-sensitive queries, pass through to real registry
    return OriginalRegQueryValueExW(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
}

// Hook: RegCloseKey
LONG WINAPI myRegCloseKeyHook(HKEY hKey)
{
    // Clean up tracking for this key
    std::map<HKEY, std::wstring>::iterator it = suspiciousKeys.find(hKey);
    if (it != suspiciousKeys.end())
    {
        wcout << L"[Hook] Closing tracked suspicious key: " << it->second << endl;
        suspiciousKeys.erase(it);
    }

    return OriginalRegCloseKey(hKey);
}

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
    cout << "[*] Anti-Registry Mining Injection started." << endl;

    HOOK_TRACE_INFO hRegOpenHook = { NULL };
    HOOK_TRACE_INFO hRegQueryHook = { NULL };
    HOOK_TRACE_INFO hRegCloseHook = { NULL };

    // Get addresses of registry functions
    HMODULE advapi32 = GetModuleHandle(TEXT("advapi32.dll"));
    if (advapi32 == nullptr)
    {
        cerr << "[!] Failed to get advapi32.dll module handle" << endl;
        return;
    }

    FARPROC regOpenAddr = GetProcAddress(advapi32, "RegOpenKeyExW");
    FARPROC regQueryAddr = GetProcAddress(advapi32, "RegQueryValueExW");
    FARPROC regCloseAddr = GetProcAddress(advapi32, "RegCloseKey");

    if (!regOpenAddr || !regQueryAddr || !regCloseAddr)
    {
        cerr << "[!] Failed to get registry function addresses" << endl;
        return;
    }

    cout << "[+] RegOpenKeyExW address: " << regOpenAddr << endl;
    cout << "[+] RegQueryValueExW address: " << regQueryAddr << endl;
    cout << "[+] RegCloseKey address: " << regCloseAddr << endl;

    // Store original function pointers
    OriginalRegOpenKeyExW = reinterpret_cast<RegOpenKeyExW_t>(regOpenAddr);
    OriginalRegQueryValueExW = reinterpret_cast<RegQueryValueExW_t>(regQueryAddr);
    OriginalRegCloseKey = reinterpret_cast<RegCloseKey_t>(regCloseAddr);

    // Install hooks
    NTSTATUS result1 = LhInstallHook(regOpenAddr, myRegOpenKeyExWHook, nullptr, &hRegOpenHook);
    NTSTATUS result2 = LhInstallHook(regQueryAddr, myRegQueryValueExWHook, nullptr, &hRegQueryHook);
    NTSTATUS result3 = LhInstallHook(regCloseAddr, myRegCloseKeyHook, nullptr, &hRegCloseHook);

    if (FAILED(result1) || FAILED(result2) || FAILED(result3))
    {
        wcerr << L"[!] Failed to install one or more hooks" << endl;
        if (FAILED(result1)) wcerr << L"    RegOpenKeyExW hook failed: " << RtlGetLastErrorString() << endl;
        if (FAILED(result2)) wcerr << L"    RegQueryValueExW hook failed: " << RtlGetLastErrorString() << endl;
        if (FAILED(result3)) wcerr << L"    RegCloseKey hook failed: " << RtlGetLastErrorString() << endl;
        return;
    }

    cout << "[+] All registry hooks installed successfully!" << endl;

    // Enable hooks for all threads
    ULONG ACLEntries[1] = { 0 };
    LhSetExclusiveACL(ACLEntries, 1, &hRegOpenHook);
    LhSetExclusiveACL(ACLEntries, 1, &hRegQueryHook);
    LhSetExclusiveACL(ACLEntries, 1, &hRegCloseHook);

    cout << "[+] Registry deception is now active!" << endl;
}