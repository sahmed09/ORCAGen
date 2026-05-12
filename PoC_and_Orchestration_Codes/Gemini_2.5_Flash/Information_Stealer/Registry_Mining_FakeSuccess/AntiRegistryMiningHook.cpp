#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <easyhook.h>
#include <random> // For random generation if needed for more complex deception
#include <map>    // <--- ADD THIS LINE for std::map

#pragma comment(lib, "EasyHook32.lib") // or EasyHook64.lib depending on platform

using namespace std;

// Original function pointers
// Change these to HOOK_TRACE_INFO structs, as EasyHook functions expect pointers to them
HOOK_TRACE_INFO hRegOpenKeyExA = { NULL };
HOOK_TRACE_INFO hRegQueryValueExA = { NULL };

// Define original function signatures
typedef LONG(WINAPI* PFN_RegOpenKeyExA)(
	HKEY hKey,
	LPCSTR lpSubKey,
	DWORD ulOptions,
	REGSAM samDesired,
	PHKEY phkResult
	);

typedef LONG(WINAPI* PFN_RegQueryValueExA)(
	HKEY hKey,
	LPCSTR lpValueName,
	LPDWORD lpReserved,
	LPDWORD lpType,
	LPBYTE lpData,
	LPDWORD lpcbData
	);

// Store the original function pointers
PFN_RegOpenKeyExA Real_RegOpenKeyExA = NULL;
PFN_RegQueryValueExA Real_RegQueryValueExA = NULL;

// --- Deception Data ---
// Map of sensitive registry values to their deceptive counterparts
std::map<std::string, std::string> deceptiveRegistryValues = {
	{"DefaultUserName", "FakeUser"},
	{"DefaultDomainName", "FakeDomain"},
	{"DefaultPassword", "FakePassword123!"},
	{"AutoAdminLogon", "0"}, // 0 for disabled
	{"fDenyTSConnections", "0"}, // 0 for RDP enabled (deceptive)
	{"PortNumber", "3389"}, // Standard RDP port
	// Add more deceptive values as needed
	{"DisplayName", "FakeSoftwareName v1.0"}
};

// --- Hook Functions ---

// Hook for RegOpenKeyExA
LONG WINAPI Detour_RegOpenKeyExA(
	HKEY hKey,
	LPCSTR lpSubKey,
	DWORD ulOptions,
	REGSAM samDesired,
	PHKEY phkResult)
{
	cout << "[Hook] RegOpenKeyExA intercepted: Key=" << hex << (ULONG_PTR)hKey << dec << ", SubKey=" << (lpSubKey ? lpSubKey : "NULL") << endl;

	// We allow the key to be opened normally so RegQueryValueExA can be called
	// However, if it's a key we want to completely deny access to, we could return an error here.
	// For this strategy, we want to proceed to RegQueryValueExA for value-level deception.

	// Call the original function to allow the key to be opened
	return Real_RegOpenKeyExA(hKey, lpSubKey, ulOptions, samDesired, phkResult);
}

// Hook for RegQueryValueExA
LONG WINAPI Detour_RegQueryValueExA(
	HKEY hKey,
	LPCSTR lpValueName,
	LPDWORD lpReserved,
	LPDWORD lpType,
	LPBYTE lpData,
	LPDWORD lpcbData)
{
	cout << "[Hook] RegQueryValueExA intercepted: ValueName=" << (lpValueName ? lpValueName : "NULL") << endl;

	std::string valueNameStr = (lpValueName ? lpValueName : "");

	// Check if the requested value name is one we want to deceive
	auto it = deceptiveRegistryValues.find(valueNameStr);
	if (it != deceptiveRegistryValues.end())
	{
		const std::string& deceptiveValue = it->second;
		cout << "[Deception] Providing fake data for '" << valueNameStr << "'" << endl;

		// Simulate success and provide fake data
		if (lpData != nullptr && lpcbData != nullptr)
		{
			DWORD requiredSize = static_cast<DWORD>(deceptiveValue.length() + 1); // +1 for null terminator

			if (*lpcbData >= requiredSize)
			{
				// Copy deceptive value to the buffer
				memcpy(lpData, deceptiveValue.c_str(), requiredSize);
				*lpcbData = requiredSize;
				if (lpType != nullptr)
				{
					*lpType = REG_SZ; // Indicate it's a string
				}
				return ERROR_SUCCESS; // Indicate success
			}
			else
			{
				// Buffer too small, return required size
				*lpcbData = requiredSize;
				return ERROR_MORE_DATA;
			}
		}
		else
		{
			// If lpData is NULL but lpcbData is provided, indicate required size for string
			if (lpcbData != nullptr)
			{
				*lpcbData = static_cast<DWORD>(deceptiveValue.length() + 1);
			}
			return ERROR_SUCCESS; // Still return success, as malware might be querying size
		}
	}
	else
	{
		// For other registry values, call the original function
		cout << "[Info] Passing through original RegQueryValueExA for '" << valueNameStr << "'" << endl;
		return Real_RegQueryValueExA(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
	}
}

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
	cout << "[*] Injection started for Registry Deception." << endl;

	// Get addresses of original functions
	HMODULE hAdvapi32 = GetModuleHandle(TEXT("advapi32.dll"));
	if (hAdvapi32 == NULL)
	{
		wcerr << L"Failed to get advapi32.dll handle. Error: " << GetLastError() << endl;
		return;
	}

	FARPROC pRegOpenKeyExA = GetProcAddress(hAdvapi32, "RegOpenKeyExA");
	FARPROC pRegQueryValueExA = GetProcAddress(hAdvapi32, "RegQueryValueExA");

	if (pRegOpenKeyExA == NULL || pRegQueryValueExA == NULL)
	{
		wcerr << L"Failed to get addresses of RegOpenKeyExA or RegQueryValueExA. Error: " << GetLastError() << endl;
		return;
	}

	// Install hook for RegOpenKeyExA
	NTSTATUS result = LhInstallHook(
		pRegOpenKeyExA,
		Detour_RegOpenKeyExA,
		NULL,
		&hRegOpenKeyExA  // Pass the address of the HOOK_TRACE_INFO struct
	);

	if (FAILED(result))
	{
		wstring s(RtlGetLastErrorString());
		wcerr << L"Failed to install hook for RegOpenKeyExA: " << s << endl;
	}
	else
	{
		cout << "RegOpenKeyExA hook installed successfully!" << endl;
		// Store the original function pointer
		Real_RegOpenKeyExA = (PFN_RegOpenKeyExA)pRegOpenKeyExA;
	}

	// Install hook for RegQueryValueExA
	result = LhInstallHook(
		pRegQueryValueExA,
		Detour_RegQueryValueExA,
		NULL,
		&hRegQueryValueExA  // Pass the address of the HOOK_TRACE_INFO struct
	);

	if (FAILED(result))
	{
		wstring s(RtlGetLastErrorString());
		wcerr << L"Failed to install hook for RegQueryValueExA: " << s << endl;
	}
	else
	{
		cout << "RegQueryValueExA hook installed successfully!" << endl;
		// Store the original function pointer
		Real_RegQueryValueExA = (PFN_RegQueryValueExA)pRegQueryValueExA;
	}

	// Activate the hooks
	// LhSetExclusiveACL expects a pointer to an array of TRACED_HOOK_HANDLE (HOOK_TRACE_INFO)
	// and the count.
	// The previous error messages were correct that you were passing a pointer to a single struct,
	// when EasyHook expects a pointer to an array of structs.
	// However, if you only have one hook to apply an ACL to, you can create a temporary array of one.
	// Or, more commonly, pass a pointer to the single HOOK_TRACE_INFO struct directly for that specific hook,
	// which EasyHook often supports as a convenience.

	// Let's use individual LhSetExclusiveACL calls for clarity and common EasyHook usage.
	ULONG ACLEntries[1] = { 0 }; // ACLs are usually applied to the current process

	// Apply ACL for RegOpenKeyExA
	result = LhSetExclusiveACL(ACLEntries, 1, &hRegOpenKeyExA);
	if (FAILED(result))
	{
		wstring s(RtlGetLastErrorString());
		wcerr << L"Failed to set exclusive ACL for RegOpenKeyExA hook: " << s << endl;
	}

	// Apply ACL for RegQueryValueExA
	result = LhSetExclusiveACL(ACLEntries, 1, &hRegQueryValueExA);
	if (FAILED(result))
	{
		wstring s(RtlGetLastErrorString());
		wcerr << L"Failed to set exclusive ACL for RegQueryValueExA hook: " << s << endl;
	}

	cout << "[*] Registry deception hooks activated." << endl;
}