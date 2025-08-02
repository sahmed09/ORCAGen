#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <easyhook.h>
#include <iostream>
#include <string>
#include <unordered_set>
#include <mutex>

#pragma comment(lib, "EasyHook32.lib")  // or EasyHook64.lib for 64-bit

using namespace std;

// --- Globals for tracking handles opened for Winlogon registry key ---
std::unordered_set<HKEY> g_winlogonHandles;
std::mutex g_handleMutex;

// --- Function Pointers for Original APIs ---
typedef LONG(WINAPI* RegOpenKeyExW_t)(
	HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult);
typedef LONG(WINAPI* RegQueryValueExW_t)(
	HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType,
	LPBYTE lpData, LPDWORD lpcbData);

RegOpenKeyExW_t True_RegOpenKeyExW = nullptr;
RegQueryValueExW_t True_RegQueryValueExW = nullptr;

// --- Helper: Case-insensitive comparison of registry paths ---
bool EndsWithInsensitive(const std::wstring& str, const std::wstring& suffix) {
	if (str.length() < suffix.length()) return false;
	return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin(),
		[](wchar_t a, wchar_t b) { return towlower(a) == towlower(b); });
}

// --- Hook: RegOpenKeyExW ---
LONG WINAPI MyRegOpenKeyExW(
	HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult)
{
	// Intercept attempts to open Winlogon key
	if (lpSubKey && EndsWithInsensitive(lpSubKey, L"Winlogon")) {
		wcout << L"[Deception] RegOpenKeyExW intercepted for Winlogon!" << endl;
		LONG ret = True_RegOpenKeyExW(hKey, lpSubKey, ulOptions, samDesired, phkResult);
		if (ret == ERROR_SUCCESS && phkResult) {
			std::lock_guard<std::mutex> lock(g_handleMutex);
			g_winlogonHandles.insert(*phkResult);
		}
		return ret;
	}
	// Otherwise, forward as normal
	return True_RegOpenKeyExW(hKey, lpSubKey, ulOptions, samDesired, phkResult);
}

// --- Hook: RegQueryValueExW ---
LONG WINAPI MyRegQueryValueExW(
	HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType,
	LPBYTE lpData, LPDWORD lpcbData)
{
	bool isWinlogon = false;
	{
		std::lock_guard<std::mutex> lock(g_handleMutex);
		if (g_winlogonHandles.count(hKey) > 0)
			isWinlogon = true;
	}
	// Target specific sensitive values
	std::vector<std::wstring> sensitiveNames = {
		L"DefaultUserName", L"DefaultPassword", L"DefaultDomainName",
		L"AutoAdminLogon", L"AltDefaultUserName", L"AltDefaultDomainName"
	};
	if (isWinlogon && lpValueName) {
		for (const auto& v : sensitiveNames) {
			if (_wcsicmp(lpValueName, v.c_str()) == 0) {
				wcout << L"[Deception] RegQueryValueExW for Winlogon/" << lpValueName << L" intercepted. Returning decoy." << endl;
				// Provide decoy value
				if (lpType)* lpType = REG_SZ;
				const wchar_t* decoy = L"DecoyValue";
				DWORD decoyLen = (DWORD)((wcslen(decoy) + 1) * sizeof(wchar_t));
				if (lpcbData) * lpcbData = decoyLen;
				if (lpData && lpcbData && *lpcbData >= decoyLen)
					memcpy(lpData, decoy, decoyLen);
				return ERROR_SUCCESS;
			}
		}
	}
	// Otherwise, forward as normal
	return True_RegQueryValueExW(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
}

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
	cout << "[*] AntiRegistryMiningHook injected." << endl;

	HOOK_TRACE_INFO hRegOpenKeyHook = { NULL };
	HOOK_TRACE_INFO hRegQueryValHook = { NULL };

	// Get original API addresses
	True_RegOpenKeyExW = (RegOpenKeyExW_t)GetProcAddress(GetModuleHandleA("advapi32.dll"), "RegOpenKeyExW");
	True_RegQueryValueExW = (RegQueryValueExW_t)GetProcAddress(GetModuleHandleA("advapi32.dll"), "RegQueryValueExW");

	// Install hooks
	LhInstallHook((LPVOID)True_RegOpenKeyExW, MyRegOpenKeyExW, nullptr, &hRegOpenKeyHook);
	LhInstallHook((LPVOID)True_RegQueryValueExW, MyRegQueryValueExW, nullptr, &hRegQueryValHook);

	// Enable all hooks (exclusive to this process)
	ULONG ACLEntries[1] = { 0 };
	LhSetExclusiveACL(ACLEntries, 1, &hRegOpenKeyHook);
	LhSetExclusiveACL(ACLEntries, 1, &hRegQueryValHook);

	cout << "[*] Registry mining deception hooks active." << endl;
}
