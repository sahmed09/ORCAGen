#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <easyhook.h>
#include <vector>
#include <ctime>

#pragma comment(lib, "EasyHook32.lib")

using namespace std;

// === Globals ===
bool g_isMalware = false;

// === Random Decoy Generator ===
std::string generateDecoyString() {
    const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::string result;
    srand((unsigned int)time(nullptr));
    for (int i = 0; i < 12; ++i)
        result += alphanum[rand() % (sizeof(alphanum) - 1)];
    return result;
}

// === Typedefs ===
typedef LONG(WINAPI* RegOpenKeyExA_t)(HKEY, LPCSTR, DWORD, REGSAM, PHKEY);
typedef LONG(WINAPI* RegQueryValueExA_t)(HKEY, LPCSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);

RegOpenKeyExA_t Original_RegOpenKeyExA = nullptr;
RegQueryValueExA_t Original_RegQueryValueExA = nullptr;

// === Hooked RegOpenKeyExA ===
LONG WINAPI MyRegOpenKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD dwReserved, REGSAM samDesired, PHKEY phkResult)
{
    if (lpSubKey != nullptr)
    {
        std::string subKeyStr = lpSubKey;
        std::string winlogon = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon";

        // Case-insensitive comparison
        if (_stricmp(subKeyStr.c_str(), winlogon.c_str()) == 0)
        {
            std::cout << "[!] Hooked RegOpenKeyExA => Winlogon key access detected.\n";
            std::cout << "    -> samDesired flags: 0x" << std::hex << samDesired << std::dec << std::endl;
            g_isMalware = true;
        }
    }

    return Original_RegOpenKeyExA(hKey, lpSubKey, dwReserved, samDesired, phkResult);
}

// === Hooked RegQueryValueExA ===
LONG WINAPI MyRegQueryValueExA(HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved,
    LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
    std::string valueName = lpValueName ? lpValueName : "NULL";

    if (g_isMalware &&
        (_stricmp(valueName.c_str(), "DefaultUserName") == 0 ||
            _stricmp(valueName.c_str(), "DefaultPassword") == 0 ||
            _stricmp(valueName.c_str(), "AutoAdminLogon") == 0))
    {
        std::string fake = generateDecoyString();
        std::cout << "[!] Returning decoy for registry value: " << valueName << " -> " << fake << std::endl;

        if (lpData && lpcbData && *lpcbData >= fake.size() + 1)
        {
            strcpy_s((char*)lpData, *lpcbData, fake.c_str());
            *lpcbData = (DWORD)(fake.size() + 1); // Include null terminator

            if (lpType)
                *lpType = REG_SZ;

            return ERROR_SUCCESS;
        }
    }

    return Original_RegQueryValueExA(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
}

// === DLL Entry Point ===
extern "C" __declspec(dllexport) void __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
    std::cout << "[+] Registry Deception DLL Injected." << std::endl;

    // Force-load advapi32 (if not already)
    LoadLibraryA("advapi32.dll");

    // Get original addresses
    FARPROC regOpenAddr = GetProcAddress(GetModuleHandleA("advapi32"), "RegOpenKeyExA");
    FARPROC regQueryAddr = GetProcAddress(GetModuleHandleA("advapi32"), "RegQueryValueExA");

    if (!regOpenAddr || !regQueryAddr)
    {
        std::cerr << "[-] Failed to locate functions in advapi32.dll!" << std::endl;
        return;
    }

    Original_RegOpenKeyExA = (RegOpenKeyExA_t)regOpenAddr;
    Original_RegQueryValueExA = (RegQueryValueExA_t)regQueryAddr;

    HOOK_TRACE_INFO hOpenHook = { 0 };
    HOOK_TRACE_INFO hQueryHook = { 0 };

    if (LhInstallHook(regOpenAddr, MyRegOpenKeyExA, nullptr, &hOpenHook) != 0)
    {
        std::cerr << "[-] Failed to hook RegOpenKeyExA!" << std::endl;
        return;
    }

    if (LhInstallHook(regQueryAddr, MyRegQueryValueExA, nullptr, &hQueryHook) != 0)
    {
        std::cerr << "[-] Failed to hook RegQueryValueExA!" << std::endl;
        return;
    }

    ULONG acl[1] = { 0 };
    LhSetExclusiveACL(acl, 1, &hOpenHook);
    LhSetExclusiveACL(acl, 1, &hQueryHook);

    std::cout << "[+] Hooks installed successfully." << std::endl;

    // Keep DLL alive
    while (true)
    {
        Sleep(1000);
    }
}
