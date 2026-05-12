#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <easyhook.h>
#include <random> // For any future random data generation, though not strictly used for registry decoys here

#pragma comment(lib, "EasyHook32.lib") // or EasyHook64.lib depending on platform

using namespace std;

// Forward declarations for original functions
// The EasyHook convention is to prefix the original function pointer with 'Original'
// for clarity, e.g., OriginalRegOpenKeyExW
static decltype(&RegOpenKeyExW) OriginalRegOpenKeyExW = nullptr;
static decltype(&RegQueryValueExW) OriginalRegQueryValueExW = nullptr;

// A global variable to hold the HKEY handle for the Winlogon key, if opened by the malware
// This helps us track if the malware is specifically querying the Winlogon key.
// Note: In a robust solution, you might map HKEYs to process/thread IDs if multiple processes are hooked
// or if you need to be more precise about which call belongs to which attempt.
// For this PoC, a simple global for one malware instance is sufficient.
HKEY g_hWinlogonKey = NULL;


// Hooked RegOpenKeyExW function
// This function will intercept calls to RegOpenKeyExW
LONG WINAPI MyRegOpenKeyExW(
	HKEY hKey,
	LPCWSTR lpSubKey,
	DWORD ulOptions,
	REGSAM samDesired,
	PHKEY phkResult
)
{
	wcout << L"[Hook] RegOpenKeyExW intercepted: hKey=";
	if (hKey == HKEY_LOCAL_MACHINE) wcout << L"HKLM";
	else if (hKey == HKEY_CURRENT_USER) wcout << L"HKCU";
	else wcout << hKey; // Print numerical value for other HKEYs
	wcout << L", SubKey=" << lpSubKey << endl;

	// Check if the malware is trying to open the specific Winlogon key
	// We check for both 64-bit and 32-bit views of the key
	if (hKey == HKEY_LOCAL_MACHINE &&
		(_wcsicmp(lpSubKey, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon") == 0 ||
			_wcsicmp(lpSubKey, L"SOFTWARE\\Wow6432Node\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon") == 0))
	{
		wcout << L"[Deception] Malware attempting to open Winlogon key. Proceeding to allow, but preparing for decoy data." << endl;
		// Call the original function to allow the key to be opened.
		// We will then use g_hWinlogonKey in RegQueryValueExW to identify subsequent queries.
		LONG result = OriginalRegOpenKeyExW(hKey, lpSubKey, ulOptions, samDesired, phkResult);
		if (result == ERROR_SUCCESS) {
			g_hWinlogonKey = *phkResult; // Store the handle for later checks
		}
		return result;
	}

	// For any other registry key access, call the original function directly.
	return OriginalRegOpenKeyExW(hKey, lpSubKey, ulOptions, samDesired, phkResult);
}


// Hooked RegQueryValueExW function
// This function will intercept calls to RegQueryValueExW
LONG WINAPI MyRegQueryValueExW(
	HKEY hKey,
	LPCWSTR lpValueName,
	LPDWORD lpReserved,
	LPDWORD lpType,
	LPBYTE lpData,
	LPDWORD lpcbData
)
{
	wcout << L"[Hook] RegQueryValueExW intercepted: hKey=" << hKey << L", ValueName=" << lpValueName << endl;

	// Check if this query is for the Winlogon key we're interested in
	if (hKey == g_hWinlogonKey)
	{
		// Intercept DefaultUserName
		if (_wcsicmp(lpValueName, L"DefaultUserName") == 0)
		{
			const WCHAR* decoyUserName = L"DecoyUser";
			DWORD decoyUserNameLen = (wcslen(decoyUserName) + 1) * sizeof(WCHAR);

			wcout << L"[Deception] Returning decoy for DefaultUserName." << endl;

			if (lpData != nullptr && *lpcbData >= decoyUserNameLen)
			{
				wcscpy_s((WCHAR*)lpData, *lpcbData / sizeof(WCHAR), decoyUserName);
				*lpcbData = decoyUserNameLen;
				if (lpType != nullptr) * lpType = REG_SZ;
				return ERROR_SUCCESS;
			}
			else if (lpData == nullptr) // Caller is asking for size
			{
				*lpcbData = decoyUserNameLen;
				if (lpType != nullptr) * lpType = REG_SZ;
				return ERROR_SUCCESS;
			}
			else // Buffer too small
			{
				*lpcbData = decoyUserNameLen; // Inform the caller of the required size
				return ERROR_MORE_DATA;
			}
		}
		// Intercept DefaultPassword
		else if (_wcsicmp(lpValueName, L"DefaultPassword") == 0)
		{
			const WCHAR* decoyPassword = L"DecoyPassword123!";
			DWORD decoyPasswordLen = (wcslen(decoyPassword) + 1) * sizeof(WCHAR);

			wcout << L"[Deception] Returning decoy for DefaultPassword." << endl;

			if (lpData != nullptr && *lpcbData >= decoyPasswordLen)
			{
				wcscpy_s((WCHAR*)lpData, *lpcbData / sizeof(WCHAR), decoyPassword);
				*lpcbData = decoyPasswordLen;
				if (lpType != nullptr) * lpType = REG_SZ;
				return ERROR_SUCCESS;
			}
			else if (lpData == nullptr) // Caller is asking for size
			{
				*lpcbData = decoyPasswordLen;
				if (lpType != nullptr) * lpType = REG_SZ;
				return ERROR_SUCCESS;
			}
			else // Buffer too small
			{
				*lpcbData = decoyPasswordLen; // Inform the caller of the required size
				return ERROR_MORE_DATA;
			}
		}
		// Intercept AutoAdminLogon
		else if (_wcsicmp(lpValueName, L"AutoAdminLogon") == 0)
		{
			DWORD decoyAutoAdminLogon = 1; // Always enabled for decoy
			DWORD decoyAutoAdminLogonSize = sizeof(DWORD);

			wcout << L"[Deception] Returning decoy for AutoAdminLogon." << endl;

			if (lpData != nullptr && *lpcbData >= decoyAutoAdminLogonSize)
			{
				*((LPDWORD)lpData) = decoyAutoAdminLogon;
				*lpcbData = decoyAutoAdminLogonSize;
				if (lpType != nullptr) * lpType = REG_DWORD;
				return ERROR_SUCCESS;
			}
			else if (lpData == nullptr) // Caller is asking for size
			{
				*lpcbData = decoyAutoAdminLogonSize;
				if (lpType != nullptr) * lpType = REG_DWORD;
				return ERROR_SUCCESS;
			}
			else // Buffer too small
			{
				*lpcbData = decoyAutoAdminLogonSize; // Inform the caller of the required size
				return ERROR_MORE_DATA;
			}
		}
	}

	// For any other value query or if it's not the Winlogon key we're tracking,
	// call the original function.
	return OriginalRegQueryValueExW(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
}

// Global hook information structures
HOOK_TRACE_INFO hRegOpenKeyExWHook = { NULL };
HOOK_TRACE_INFO hRegQueryValueExWHook = { NULL };

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
	cout << "[*] AntiClipboardLoggerHook: Injection started." << endl;

	// Get original function addresses
	OriginalRegOpenKeyExW = (decltype(&RegOpenKeyExW))GetProcAddress(GetModuleHandle(TEXT("advapi32")), "RegOpenKeyExW");
	OriginalRegQueryValueExW = (decltype(&RegQueryValueExW))GetProcAddress(GetModuleHandle(TEXT("advapi32")), "RegQueryValueExW");

	if (!OriginalRegOpenKeyExW || !OriginalRegQueryValueExW) {
		wcerr << L"[-] AntiClipboardLoggerHook: Could not find original registry API addresses." << endl;
		return;
	}

	cout << "[+] AntiClipboardLoggerHook: Original RegOpenKeyExW address: " << (void*)OriginalRegOpenKeyExW << endl;
	cout << "[+] AntiClipboardLoggerHook: Original RegQueryValueExW address: " << (void*)OriginalRegQueryValueExW << endl;


	// Install hooks for RegOpenKeyExW and RegQueryValueExW
	NTSTATUS result = LhInstallHook(
		OriginalRegOpenKeyExW,
		MyRegOpenKeyExW,
		nullptr,
		&hRegOpenKeyExWHook
	);

	if (FAILED(result))
	{
		wstring s(RtlGetLastErrorString());
		wcerr << L"[-] AntiClipboardLoggerHook: Failed to install RegOpenKeyExW hook: " << s << endl;
	}
	else
	{
		cout << "[+] AntiClipboardLoggerHook: RegOpenKeyExW hook installed successfully!" << endl;
	}

	result = LhInstallHook(
		OriginalRegQueryValueExW,
		MyRegQueryValueExW,
		nullptr,
		&hRegQueryValueExWHook
	);

	if (FAILED(result))
	{
		wstring s(RtlGetLastErrorString());
		wcerr << L"[-] AntiClipboardLoggerHook: Failed to install RegQueryValueExW hook: " << s << endl;
	}
	else
	{
		cout << "[+] AntiClipboardLoggerHook: RegQueryValueExW hook installed successfully!" << endl;
	}

	// Set ACLs to enable the hooks for all threads in the target process
	// This is crucial for the hooks to become active.
	ULONG ACLEntries[1] = { 0 }; // 0 means all threads
	if (SUCCEEDED(result)) { // Only set ACL if the last hook installation was successful
		LhSetExclusiveACL(ACLEntries, 1, &hRegOpenKeyExWHook);
		LhSetExclusiveACL(ACLEntries, 1, &hRegQueryValueExWHook);
		cout << "[+] AntiClipboardLoggerHook: Registry API hooks enabled for all threads." << endl;
	}
}

// In a real application, you might want to consider unhooking in DllMain(DLL_PROCESS_DETACH)
// LhUninstallHook(&hRegOpenKeyExWHook);
// LhUninstallHook(&hRegQueryValueExWHook);
// LhWaitForPendingRemovals();