#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <easyhook.h>

#pragma comment(lib, "EasyHook32.lib") // Use EasyHook64.lib for 64-bit

using namespace std;

// Saved original function pointers
typedef LSTATUS(WINAPI* RegOpenKeyExW_t)(HKEY, LPCWSTR, DWORD, REGSAM, PHKEY);
RegOpenKeyExW_t True_RegOpenKeyExW = nullptr;

typedef LSTATUS(WINAPI* RegQueryValueExW_t)(HKEY, LPCWSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
RegQueryValueExW_t True_RegQueryValueExW = nullptr;

// Globals to track "special" opened keys
std::wstring g_winlogon_key = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon";

// Intercepted RegOpenKeyExW
LSTATUS WINAPI myRegOpenKeyExW(
	HKEY hKey,
	LPCWSTR lpSubKey,
	DWORD ulOptions,
	REGSAM samDesired,
	PHKEY phkResult
)
{
	// If the malware tries to open the Winlogon key
	if (lpSubKey && _wcsicmp(lpSubKey, g_winlogon_key.c_str()) == 0) {
		wcout << L"[Hook] Blocked RegOpenKeyExW on Winlogon: " << lpSubKey << endl;
		// Fake a "file not found" error (malware will think the key doesn't exist)
		return ERROR_FILE_NOT_FOUND;
	}
	// Otherwise, pass through
	return True_RegOpenKeyExW(hKey, lpSubKey, ulOptions, samDesired, phkResult);
}

// Intercepted RegQueryValueExW
LSTATUS WINAPI myRegQueryValueExW(
	HKEY hKey,
	LPCWSTR lpValueName,
	LPDWORD lpReserved,
	LPDWORD lpType,
	LPBYTE lpData,
	LPDWORD lpcbData
)
{
	// Protect specific value names
	if (lpValueName) {
		std::wstring valName(lpValueName);
		if (
			valName == L"DefaultPassword" ||
			valName == L"DefaultUserName" ||
			valName == L"DefaultDomainName" ||
			valName == L"AltDefaultUserName" ||
			valName == L"AltDefaultDomainName" ||
			valName == L"AutoAdminLogon"
			) {
			wcout << L"[Hook] Blocked RegQueryValueExW on value: " << valName << endl;
			// Fake an error (malware thinks value doesn't exist or access denied)
			return ERROR_ACCESS_DENIED;
		}
	}
	// Otherwise, pass through
	return True_RegQueryValueExW(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
}

// Entry point for EasyHook injection
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO* inRemoteInfo)
{
	cout << "[*] Registry deception DLL injected." << endl;

	HOOK_TRACE_INFO hRegOpenKeyHook = { NULL };
	HOOK_TRACE_INFO hRegQueryValueHook = { NULL };

	HMODULE hAdvapi = GetModuleHandleW(L"advapi32.dll");
	if (!hAdvapi) {
		cout << "[!] Failed to get advapi32.dll handle." << endl;
		return;
	}

	// Install hooks
	True_RegOpenKeyExW = (RegOpenKeyExW_t)GetProcAddress(hAdvapi, "RegOpenKeyExW");
	True_RegQueryValueExW = (RegQueryValueExW_t)GetProcAddress(hAdvapi, "RegQueryValueExW");

	if (!True_RegOpenKeyExW || !True_RegQueryValueExW) {
		cout << "[!] Failed to resolve original registry API addresses." << endl;
		return;
	}

	NTSTATUS res1 = LhInstallHook(
		True_RegOpenKeyExW,
		myRegOpenKeyExW,
		nullptr,
		&hRegOpenKeyHook);

	NTSTATUS res2 = LhInstallHook(
		True_RegQueryValueExW,
		myRegQueryValueExW,
		nullptr,
		&hRegQueryValueHook);

	if (FAILED(res1) || FAILED(res2)) {
		wcout << L"[!] Failed to install registry hooks: "
			<< RtlGetLastErrorString() << endl;
		return;
	}

	ULONG ACLEntries[1] = { 0 }; // Exclusive: only current process
	LhSetExclusiveACL(ACLEntries, 1, &hRegOpenKeyHook);
	LhSetExclusiveACL(ACLEntries, 1, &hRegQueryValueHook);

	cout << "[+] Registry hooks installed successfully." << endl;
}
