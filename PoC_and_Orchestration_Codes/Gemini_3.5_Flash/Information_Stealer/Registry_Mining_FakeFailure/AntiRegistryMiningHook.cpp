#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winreg.h>
#include <iostream>
#include <string>
#include <map>
#include <mutex>
#include <easyhook.h>

#pragma comment(lib, "EasyHook32.lib") // Use EasyHook32.lib if compiling for a 32-bit process

using namespace std;

// Thread-safe map to track handles associated with sensitive subkeys
std::mutex g_RegistryMutex;
std::map<HKEY, std::string> g_TrackedKeys;

// Function pointers for the original Windows APIs
typedef LONG(WINAPI* RegOpenKeyExA_pfn)(HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult);
typedef LONG(WINAPI* RegQueryValueExA_pfn)(HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
typedef LONG(WINAPI* RegCloseKey_pfn)(HKEY hKey);

RegOpenKeyExA_pfn Original_RegOpenKeyExA = nullptr;
RegQueryValueExA_pfn Original_RegQueryValueExA = nullptr;
RegCloseKey_pfn Original_RegCloseKey = nullptr;

// 1. Detour for RegOpenKeyExA
LONG WINAPI Detour_RegOpenKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult)
{
    if (lpSubKey != nullptr)
    {
        std::string subKeyStr(lpSubKey);

        // Intercept requests targeting sensitive configuration paths
        if (subKeyStr.find("Winlogon") != std::string::npos)
        {
            cout << "[Active Defense] Intercepted RegOpenKeyExA for path: " << subKeyStr << endl;

            // OPTION A: Fail immediately at the open stage
            // SetLastError(ERROR_ACCESS_DENIED);
            // return ERROR_ACCESS_DENIED;

            // OPTION B: Allow the open to succeed but track the handle to fail specific queries later
            LONG result = Original_RegOpenKeyExA(hKey, lpSubKey, ulOptions, samDesired, phkResult);
            if (result == ERROR_SUCCESS && phkResult != nullptr)
            {
                std::lock_guard<std::mutex> lock(g_RegistryMutex);
                g_TrackedKeys[*phkResult] = subKeyStr;
            }
            return result;
        }
    }

    return Original_RegOpenKeyExA(hKey, lpSubKey, ulOptions, samDesired, phkResult);
}

// 2. Detour for RegQueryValueExA
LONG WINAPI Detour_RegQueryValueExA(HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
    std::string trackedPath = "";
    {
        std::lock_guard<std::mutex> lock(g_RegistryMutex);
        auto it = g_TrackedKeys.find(hKey);
        if (it != g_TrackedKeys.end())
        {
            trackedPath = it->second;
        }
    }

    // If the handle belongs to a protected path, simulate a lookup failure
    if (!trackedPath.empty() && lpValueName != nullptr)
    {
        std::string valName(lpValueName);

        // Deny access to specific properties within the Winlogon key
        if (valName == "DefaultPassword" || valName == "DefaultUserName" || valName == "AutoAdminLogon")
        {
            cout << "[FakeFailure] Denying query for value: " << valName << " under " << trackedPath << endl;

            // Simulate that the value does not exist in the registry
            SetLastError(ERROR_FILE_NOT_FOUND);
            return ERROR_FILE_NOT_FOUND;
        }
    }

    // Pass through for all other untargeted registry queries
    return Original_RegQueryValueExA(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
}

// 3. Detour for RegCloseKey
LONG WINAPI Detour_RegCloseKey(HKEY hKey)
{
    {
        std::lock_guard<std::mutex> lock(g_RegistryMutex);
        g_TrackedKeys.erase(hKey);
    }
    return Original_RegCloseKey(hKey);
}

// Injection Entry Point
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
    cout << "[*] Active Defense DLL Loaded inside target process." << endl;

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

        // Install hooks using EasyHook
        LhInstallHook((FARPROC)Original_RegOpenKeyExA, Detour_RegOpenKeyExA, nullptr, &hRegOpen);
        LhInstallHook((FARPROC)Original_RegQueryValueExA, Detour_RegQueryValueExA, nullptr, &hRegQuery);
        LhInstallHook((FARPROC)Original_RegCloseKey, Detour_RegCloseKey, nullptr, &hRegClose);

        // Enable hooks for all threads in the current process
        ULONG ACLEntries[1] = { 0 };
        LhSetExclusiveACL(ACLEntries, 1, &hRegOpen);
        LhSetExclusiveACL(ACLEntries, 1, &hRegQuery);
        LhSetExclusiveACL(ACLEntries, 1, &hRegClose);

        cout << "[+] FakeFailure hooks successfully established." << endl;
    }
    else
    {
        cerr << "[-] Failed to locate advapi32.dll." << endl;
    }
}