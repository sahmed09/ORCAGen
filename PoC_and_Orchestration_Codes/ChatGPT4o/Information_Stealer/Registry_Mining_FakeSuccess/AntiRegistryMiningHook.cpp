#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <unordered_map>
#include <easyhook.h>

#pragma comment(lib, "EasyHook32.lib")

using namespace std;

HKEY gFakeKey = reinterpret_cast<HKEY>(0xBAADF00D);  // Dummy fake key for deception
HOOK_TRACE_INFO hRegOpenKeyHook = { NULL };
HOOK_TRACE_INFO hRegQueryValueHook = { NULL };

// Save fake values
unordered_map<wstring, wstring> fakeRegistryData = {
	{L"DefaultUserName", L"sys_admin"},
	{L"DefaultPassword", L"Pa$$w0rd123!"},
	{L"DefaultDomainName", L"CORPDOMAIN"},
	{L"AutoAdminLogon", L"1"}
};

// Original function pointers
decltype(&RegOpenKeyExW) true_RegOpenKeyExW = RegOpenKeyExW;
decltype(&RegQueryValueExW) true_RegQueryValueExW = RegQueryValueExW;

// Hooked RegOpenKeyExW
LSTATUS WINAPI myRegOpenKeyExW(
	HKEY hKey,
	LPCWSTR lpSubKey,
	DWORD ulOptions,
	REGSAM samDesired,
	PHKEY phkResult)
{
	if (hKey == HKEY_LOCAL_MACHINE &&
		lpSubKey != nullptr &&
		wcsstr(lpSubKey, L"Winlogon") != nullptr)
	{
		wcout << L"[Hook] RegOpenKeyExW intercepted for: " << lpSubKey << endl;
		*phkResult = gFakeKey;
		return ERROR_SUCCESS;  // Fake success
	}

	return true_RegOpenKeyExW(hKey, lpSubKey, ulOptions, samDesired, phkResult);
}

// Hooked RegQueryValueExW
LSTATUS WINAPI myRegQueryValueExW(
	HKEY hKey,
	LPCWSTR lpValueName,
	LPDWORD lpReserved,
	LPDWORD lpType,
	LPBYTE lpData,
	LPDWORD lpcbData)
{
	if (hKey == gFakeKey && lpValueName != nullptr)
	{
		wcout << L"[Hook] RegQueryValueExW intercepted for: " << lpValueName << endl;

		auto it = fakeRegistryData.find(lpValueName);
		if (it != fakeRegistryData.end())
		{
			const wstring& fakeValue = it->second;
			size_t sizeInBytes = (fakeValue.size() + 1) * sizeof(wchar_t);

			if (lpData && lpcbData && *lpcbData >= sizeInBytes)
			{
				memcpy(lpData, fakeValue.c_str(), sizeInBytes);
				if (lpType)* lpType = REG_SZ;
				*lpcbData = static_cast<DWORD>(sizeInBytes);
				return ERROR_SUCCESS;
			}
			else
			{
				*lpcbData = static_cast<DWORD>(sizeInBytes);
				return ERROR_MORE_DATA;
			}
		}

		return ERROR_FILE_NOT_FOUND;
	}

	return true_RegQueryValueExW(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
}

// Entry Point
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO* inRemoteInfo)
{
	cout << "[*] Injection started: Registry deception active." << endl;

	// Install hooks
	LhInstallHook(
		GetProcAddress(GetModuleHandleW(L"advapi32.dll"), "RegOpenKeyExW"),
		myRegOpenKeyExW,
		nullptr,
		&hRegOpenKeyHook);

	LhInstallHook(
		GetProcAddress(GetModuleHandleW(L"advapi32.dll"), "RegQueryValueExW"),
		myRegQueryValueExW,
		nullptr,
		&hRegQueryValueHook);

	// Apply ACL to all threads (except the injector)
	ULONG ACLEntries[1] = { 0 };
	LhSetExclusiveACL(ACLEntries, 1, &hRegOpenKeyHook);
	LhSetExclusiveACL(ACLEntries, 1, &hRegQueryValueHook);
}
