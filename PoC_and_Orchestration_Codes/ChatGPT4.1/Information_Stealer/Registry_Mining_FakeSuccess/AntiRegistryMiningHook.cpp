#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winreg.h>
#include <iostream>
#include <string>
#include <easyhook.h>

#pragma comment(lib, "EasyHook32.lib")

// Global variables to identify fake key handles
static HKEY g_FakeWinlogonKey = (HKEY)0xDEADBEEF; // Arbitrary value, unlikely to clash

// Function pointers for original APIs
typedef LONG(WINAPI* RegOpenKeyExW_t)(HKEY, LPCWSTR, DWORD, REGSAM, PHKEY);
typedef LONG(WINAPI* RegQueryValueExW_t)(HKEY, LPCWSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);

RegOpenKeyExW_t TrueRegOpenKeyExW = nullptr;
RegQueryValueExW_t TrueRegQueryValueExW = nullptr;

// Intercepted RegOpenKeyExW
LONG WINAPI myRegOpenKeyExWHook(
	HKEY hKey,
	LPCWSTR lpSubKey,
	DWORD ulOptions,
	REGSAM samDesired,
	PHKEY phkResult
) {
	if (
		hKey == HKEY_LOCAL_MACHINE &&
		lpSubKey &&
		_wcsicmp(lpSubKey, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon") == 0
		) {
		// Return fake handle for the Winlogon key
		*phkResult = g_FakeWinlogonKey;
		std::wcout << L"[Deception] RegOpenKeyExW intercepted Winlogon, returning fake handle." << std::endl;
		return ERROR_SUCCESS;
	}
	// Otherwise, call the real function
	return TrueRegOpenKeyExW(hKey, lpSubKey, ulOptions, samDesired, phkResult);
}

// Intercepted RegQueryValueExW
LONG WINAPI myRegQueryValueExWHook(
	HKEY hKey,
	LPCWSTR lpValueName,
	LPDWORD lpReserved,
	LPDWORD lpType,
	LPBYTE lpData,
	LPDWORD lpcbData
) {
	// Intercept only for our fake handle and protected value names
	if (hKey == g_FakeWinlogonKey && lpValueName) {
		std::wstring val(lpValueName);
		if (
			val == L"DefaultPassword" ||
			val == L"DefaultUserName" ||
			val == L"DefaultDomainName" ||
			val == L"AutoAdminLogon" ||
			val == L"AltDefaultUserName"
			) {
			std::wstring fakeValue = L"********"; // You can vary this value for more deception
			DWORD fakeType = REG_SZ;
			DWORD bytesNeeded = (DWORD)((fakeValue.length() + 1) * sizeof(wchar_t));
			if (lpData && lpcbData && *lpcbData >= bytesNeeded) {
				memcpy(lpData, fakeValue.c_str(), bytesNeeded);
				*lpType = fakeType;
				*lpcbData = bytesNeeded;
				std::wcout << L"[Deception] RegQueryValueExW intercepted " << val << L", returning fake data." << std::endl;
				return ERROR_SUCCESS;
			}
			else if (lpcbData) {
				*lpcbData = bytesNeeded;
				if (lpType)* lpType = fakeType;
				return ERROR_MORE_DATA;
			}
		}
	}
	// Otherwise, call the real function
	return TrueRegQueryValueExW(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
}

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO* inRemoteInfo)
{
	std::cout << "[*] Injection started for Registry Mining Deception." << std::endl;

	// Resolve original API addresses
	TrueRegOpenKeyExW = (RegOpenKeyExW_t)GetProcAddress(GetModuleHandleW(L"advapi32"), "RegOpenKeyExW");
	TrueRegQueryValueExW = (RegQueryValueExW_t)GetProcAddress(GetModuleHandleW(L"advapi32"), "RegQueryValueExW");

	HOOK_TRACE_INFO hRegOpenKeyExHook = { NULL };
	HOOK_TRACE_INFO hRegQueryValueExHook = { NULL };

	// Install hooks
	LhInstallHook((LPVOID)TrueRegOpenKeyExW, myRegOpenKeyExWHook, nullptr, &hRegOpenKeyExHook);
	LhInstallHook((LPVOID)TrueRegQueryValueExW, myRegQueryValueExWHook, nullptr, &hRegQueryValueExHook);

	// Enable for all threads in current process
	ULONG ACLEntries[1] = { 0 };
	LhSetExclusiveACL(ACLEntries, 1, &hRegOpenKeyExHook);
	LhSetExclusiveACL(ACLEntries, 1, &hRegQueryValueExHook);

	std::cout << "[*] Registry API hooks installed.\n";
}
