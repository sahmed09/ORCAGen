#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winreg.h>
#include <easyhook.h>
#include <iostream>
#include <string>

#pragma comment(lib, "EasyHook32.lib") // or EasyHook64.lib depending on platform

using namespace std;

// === Globals ===
HOOK_TRACE_INFO hOpenKeyHook = { NULL };
HOOK_TRACE_INFO hQueryValueHook = { NULL };

// Track the handle of the deceptive Winlogon key
HKEY g_FakeWinlogonKey = nullptr;

// === Original function pointers ===
typedef LSTATUS(WINAPI* RegOpenKeyExW_t)(
	HKEY hKey,
	LPCWSTR lpSubKey,
	DWORD ulOptions,
	REGSAM samDesired,
	PHKEY phkResult
	);
RegOpenKeyExW_t TrueRegOpenKeyExW = RegOpenKeyExW;

typedef LSTATUS(WINAPI* RegQueryValueExW_t)(
	HKEY hKey,
	LPCWSTR lpValueName,
	LPDWORD lpReserved,
	LPDWORD lpType,
	LPBYTE lpData,
	LPDWORD lpcbData
	);
RegQueryValueExW_t TrueRegQueryValueExW = RegQueryValueExW;

// === Hooked RegOpenKeyExW ===
LSTATUS WINAPI myRegOpenKeyExW(
	HKEY hKey,
	LPCWSTR lpSubKey,
	DWORD ulOptions,
	REGSAM samDesired,
	PHKEY phkResult
)
{
	if (hKey == HKEY_LOCAL_MACHINE &&
		lpSubKey != nullptr &&
		_wcsicmp(lpSubKey, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon") == 0)
	{
		wcout << L"[Hook] RegOpenKeyExW intercepted for Winlogon key." << endl;
		// Create a fake key handle (can be anything but not null)
		g_FakeWinlogonKey = (HKEY)0xBEEF1234;
		*phkResult = g_FakeWinlogonKey;
		return ERROR_SUCCESS;
	}

	return TrueRegOpenKeyExW(hKey, lpSubKey, ulOptions, samDesired, phkResult);
}

// === Hooked RegQueryValueExW ===
LSTATUS WINAPI myRegQueryValueExW(
	HKEY hKey,
	LPCWSTR lpValueName,
	LPDWORD lpReserved,
	LPDWORD lpType,
	LPBYTE lpData,
	LPDWORD lpcbData
)
{
	if (hKey == g_FakeWinlogonKey && lpValueName != nullptr)
	{
		wstring valueName(lpValueName);
		wcout << L"[Hook] RegQueryValueExW intercepted for " << valueName << endl;

		const wchar_t* decoyValue = L"Decoy123";
		DWORD decoyLen = (wcslen(decoyValue) + 1) * sizeof(wchar_t);

		if (lpData && *lpcbData >= decoyLen)
		{
			memcpy(lpData, decoyValue, decoyLen);
		}

		if (lpcbData)
			* lpcbData = decoyLen;

		if (lpType)
			* lpType = REG_SZ;

		return ERROR_SUCCESS;
	}

	return TrueRegQueryValueExW(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
}


// === NativeInjectionEntryPoint ===
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO* inRemoteInfo)
{
	cout << "[*] Registry deception injection started." << endl;

	// Install hook for RegOpenKeyExW
	FARPROC openKeyAddr = GetProcAddress(GetModuleHandle(TEXT("advapi32")), "RegOpenKeyExW");
	LhInstallHook(openKeyAddr, myRegOpenKeyExW, nullptr, &hOpenKeyHook);

	// Install hook for RegQueryValueExW
	FARPROC queryValueAddr = GetProcAddress(GetModuleHandle(TEXT("advapi32")), "RegQueryValueExW");
	LhInstallHook(queryValueAddr, myRegQueryValueExW, nullptr, &hQueryValueHook);

	// Restrict hooks to current process
	ULONG ACLEntries[1] = { 0 };
	LhSetExclusiveACL(ACLEntries, 1, &hOpenKeyHook);
	LhSetExclusiveACL(ACLEntries, 1, &hQueryValueHook);

	cout << "[*] Registry deception hooks installed successfully." << endl;
}
