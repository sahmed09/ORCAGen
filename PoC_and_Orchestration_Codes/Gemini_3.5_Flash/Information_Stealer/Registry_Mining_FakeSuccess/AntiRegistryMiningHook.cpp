#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winreg.h>
#include <iostream>
#include <string>
#include <map>
#include <mutex>
#include <easyhook.h>

#pragma comment(lib, "EasyHook32.lib") // Swap to EasyHook32.lib if your target environment is x86

using namespace std;

// Mutex and structural map to dynamically keep track of open HKEY handle contexts
std::mutex g_RegistryMutex;
std::map<HKEY, std::string> g_TrackedKeys;

// Typedef definitions matching the original ANSI Windows API exports
typedef LONG(WINAPI* RegOpenKeyExA_pfn)(HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult);
typedef LONG(WINAPI* RegQueryValueExA_pfn)(HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
typedef LONG(WINAPI* RegCloseKey_pfn)(HKEY hKey);

RegOpenKeyExA_pfn Original_RegOpenKeyExA = nullptr;
RegQueryValueExA_pfn Original_RegQueryValueExA = nullptr;
RegCloseKey_pfn Original_RegCloseKey = nullptr;

// 1. Hook for RegOpenKeyExA
LONG WINAPI Detour_RegOpenKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult)
{
    // Execute original API path to acquire a valid system-assigned handle structural layout
    LONG result = Original_RegOpenKeyExA(hKey, lpSubKey, ulOptions, samDesired, phkResult);

    if (result == ERROR_SUCCESS && lpSubKey != nullptr && phkResult != nullptr)
    {
        std::string subKeyStr(lpSubKey);

        // Audit targeting footprints for sensitive targets outlined in mining profiles
        if (subKeyStr.find("Winlogon") != std::string::npos ||
            subKeyStr.find("Terminal Server") != std::string::npos ||
            subKeyStr.find("Uninstall") != std::string::npos)
        {
            cout << "[Active Defense] Intercepted RegOpenKeyExA mapping to target path: " << subKeyStr << endl;

            std::lock_guard<std::mutex> lock(g_RegistryMutex);
            g_TrackedKeys[*phkResult] = subKeyStr;
        }
    }
    return result;
}

// Helper to simulate a clean data return for FakeSuccess requirements
LONG PopulateFakeString(LPBYTE lpData, LPDWORD lpcbData, LPDWORD lpType, const std::string& fakeValue)
{
    DWORD requiredSize = (DWORD)fakeValue.length() + 1;
    if (lpType) *lpType = REG_SZ;

    if (lpData == nullptr) // Caller calculating expected sizing allocations
    {
        *lpcbData = requiredSize;
        return ERROR_SUCCESS;
    }
    if (*lpcbData < requiredSize)
    {
        *lpcbData = requiredSize;
        return ERROR_MORE_DATA;
    }

    strcpy_s((char*)lpData, *lpcbData, fakeValue.c_str());
    *lpcbData = requiredSize;
    return ERROR_SUCCESS;
}

// 2. Hook for RegQueryValueExA
LONG WINAPI Detour_RegQueryValueExA(HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
    std::string keyPath = "";
    {
        std::lock_guard<std::mutex> lock(g_RegistryMutex);
        auto it = g_TrackedKeys.find(hKey);
        if (it != g_TrackedKeys.end())
        {
            keyPath = it->second;
        }
    }

    // Process-exclusive validation check for targeted sub-keys
    if (!keyPath.empty() && lpValueName != nullptr)
    {
        std::string valName(lpValueName);

        // Scenario 1: Deceptive updates targeting Credential Harvesting (Winlogon)
        if (keyPath.find("Winlogon") != std::string::npos)
        {
            cout << "[FakeSuccess] Spoofing Winlogon query -> " << valName << endl;
            if (valName == "DefaultUserName")   return PopulateFakeString(lpData, lpcbData, lpType, "honey_operator");
            if (valName == "DefaultDomainName") return PopulateFakeString(lpData, lpcbData, lpType, "HONEYNET_LABS");
            if (valName == "DefaultPassword")   return PopulateFakeString(lpData, lpcbData, lpType, "DecoyCredential2026!");
            if (valName == "AutoAdminLogon")    return PopulateFakeString(lpData, lpcbData, lpType, "1");
        }

        // Scenario 2: Deceptive updates targeting Lateral Movement Scouting (RDP Configuration)
        else if (keyPath.find("Terminal Server") != std::string::npos)
        {
            cout << "[FakeSuccess] Spoofing RDP target query -> " << valName << endl;
            if (valName == "fDenyTSConnections") return PopulateFakeString(lpData, lpcbData, lpType, "1"); // Report disabled
            if (valName == "PortNumber")
            {
                if (lpType) *lpType = REG_DWORD;
                if (lpData != nullptr)
                {
                    if (*lpcbData >= sizeof(DWORD)) { *(DWORD*)lpData = 3389; *lpcbData = sizeof(DWORD); }
                    else return ERROR_MORE_DATA;
                }
                else { *lpcbData = sizeof(DWORD); }
                return ERROR_SUCCESS;
            }
        }

        // Scenario 3: Deceptive updates targeting Environmental Reconnaissance (Installed Software)
        else if (keyPath.find("Uninstall") != std::string::npos)
        {
            cout << "[FakeSuccess] Spoofing Installation Registry probe -> " << valName << endl;
            if (valName == "DisplayName") return PopulateFakeString(lpData, lpcbData, lpType, "SecureAgent_Decoy_v4.2");
        }
    }

    // Smooth cascading fallback layout ensuring normal operability across untargeted space
    return Original_RegQueryValueExA(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
}

// 3. Hook for RegCloseKey
LONG WINAPI Detour_RegCloseKey(HKEY hKey)
{
    {
        std::lock_guard<std::mutex> lock(g_RegistryMutex);
        g_TrackedKeys.erase(hKey);
    }
    return Original_RegCloseKey(hKey);
}

// Entry point initialization orchestrating standard engine integration pipeline
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
    cout << "[*] Cyber Deception Active: Monitoring registry mining paths." << endl;

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
        Original_RegOpenKeyExA = (RegOpenKeyExA_pfn)GetProcAddress(hAdvapi32, "RegOpenKeyExA");
        Original_RegQueryValueExA = (RegQueryValueExA_pfn)GetProcAddress(hAdvapi32, "RegQueryValueExA");
        Original_RegCloseKey = (RegCloseKey_pfn)GetProcAddress(hAdvapi32, "RegCloseKey");

        // Hook execution via EasyHook mechanics
        LhInstallHook((FARPROC)Original_RegOpenKeyExA, Detour_RegOpenKeyExA, nullptr, &hRegOpen);
        LhInstallHook((FARPROC)Original_RegQueryValueExA, Detour_RegQueryValueExA, nullptr, &hRegQuery);
        LhInstallHook((FARPROC)Original_RegCloseKey, Detour_RegCloseKey, nullptr, &hRegClose);

        // Bind interception rights universally across thread landscapes inside target boundary
        ULONG ACLEntries[1] = { 0 };
        LhSetExclusiveACL(ACLEntries, 1, &hRegOpen);
        LhSetExclusiveACL(ACLEntries, 1, &hRegQuery);
        LhSetExclusiveACL(ACLEntries, 1, &hRegClose);

        cout << "[+] Active defense hooks successfully mounted onto Advapi32 ANSI APIs." << endl;
    }
    else
    {
        cerr << "[-] Error updating context blocks: Advapi32 resolution failed." << endl;
    }
}