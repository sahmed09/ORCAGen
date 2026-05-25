#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winreg.h>
#include <iostream>
#include <string>
#include <easyhook.h>

#pragma comment(lib, "EasyHook32.lib") // or EasyHook64.lib depending on your target

using namespace std;

HOOK_TRACE_INFO hRegOpenHook = { NULL };
HOOK_TRACE_INFO hRegQueryHook = { NULL };

// Store handle to fake detection
HKEY gDeceptiveKey = NULL;

// Target path for deception (must match the one in malware)
const std::wstring deceptiveSubkey = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon";

// Hooked RegOpenKeyExW
LONG WINAPI myRegOpenKeyExW(
	HKEY hKey,
	LPCWSTR lpSubKey,
	DWORD ulOptions,
	REGSAM samDesired,
	PHKEY phkResult
)
{
	wstring fullKey = (hKey == HKEY_LOCAL_MACHINE ? L"HKLM\\" : L"OTHER\\") + wstring(lpSubKey ? lpSubKey : L"");

	if (fullKey.find(deceptiveSubkey) != wstring::npos) {
		wcout << L"[Deception] Blocked RegOpenKeyExW access to: " << fullKey << endl;
		*phkResult = gDeceptiveKey = reinterpret_cast<HKEY>(0xDEADBEEF); // dummy key handle
		return ERROR_SUCCESS;
	}

	return RegOpenKeyExW(hKey, lpSubKey, ulOptions, samDesired, phkResult);
}

// Hooked RegQueryValueExW
LONG WINAPI myRegQueryValueExW(
	HKEY hKey,
	LPCWSTR lpValueName,
	LPDWORD lpReserved,
	LPDWORD lpType,
	LPBYTE lpData,
	LPDWORD lpcbData
)
{
	if (hKey == gDeceptiveKey) {
		wcout << L"[Deception] Blocked RegQueryValueExW for: " << lpValueName << endl;
		return ERROR_ACCESS_DENIED;
	}

	return RegQueryValueExW(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
}

// DLL entry point for injection
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO* inRemoteInfo)
{
	cout << "[*] Injection started." << endl;

	// Get function addresses
	FARPROC regOpenAddr = GetProcAddress(GetModuleHandle(TEXT("advapi32")), "RegOpenKeyExW");
	FARPROC regQueryAddr = GetProcAddress(GetModuleHandle(TEXT("advapi32")), "RegQueryValueExW");

	// Install hooks
	if (LhInstallHook(regOpenAddr, myRegOpenKeyExW, nullptr, &hRegOpenHook) == 0)
		cout << "[+] Hook installed: RegOpenKeyExW" << endl;
	else
		wcerr << L"[!] Failed to hook RegOpenKeyExW: " << RtlGetLastErrorString() << endl;

	if (LhInstallHook(regQueryAddr, myRegQueryValueExW, nullptr, &hRegQueryHook) == 0)
		cout << "[+] Hook installed: RegQueryValueExW" << endl;
	else
		wcerr << L"[!] Failed to hook RegQueryValueExW: " << RtlGetLastErrorString() << endl;

	// Apply hooks to all threads in the target process
	ULONG ACLEntries[1] = { 0 };
	LhSetExclusiveACL(ACLEntries, 1, &hRegOpenHook);
	LhSetExclusiveACL(ACLEntries, 1, &hRegQueryHook);
}
