#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <easyhook.h>
#include <random>
#include <winreg.h> // Required for registry API definitions

#pragma comment(lib, "EasyHook32.lib") // Adjust for 64-bit if compiling for x64

using namespace std;

// Random generator (from sample, keeping for consistency)
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dist(0, 99);

// Trampoline for original RegOpenKeyExA
typedef LONG(WINAPI* FnRegOpenKeyExA)(HKEY, LPCSTR, DWORD, REGSAM, PHKEY);
FnRegOpenKeyExA g_pOriginalRegOpenKeyExA = nullptr;

// Trampoline for original RegQueryValueExA
typedef LONG(WINAPI* FnRegQueryValueExA)(HKEY, LPCSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
FnRegQueryValueExA g_pOriginalRegQueryValueExA = nullptr;

// Flag to track if we're currently in the sensitive key context
bool g_isInWinlogonContext = false;

// Hook function for RegOpenKeyExA
LONG WINAPI MyRegOpenKeyExAHook(
	HKEY hKey,
	LPCSTR lpSubKey,
	DWORD ulOptions,
	REGSAM samDesired,
	PHKEY phkResult
)
{
	std::cout << "[Hook] RegOpenKeyExA intercepted. Key: ";
	if (hKey == HKEY_LOCAL_MACHINE) std::cout << "HKLM\\";
	std::cout << (lpSubKey ? lpSubKey : "") << std::endl;

	// Check if the malware is attempting to open the Winlogon key
	if (lpSubKey && (
		_stricmp(lpSubKey, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon") == 0 ||
		_stricmp(lpSubKey, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon") == 0
		)
		)
	{
		std::cout << "[Deception] Malware attempting to open Winlogon key. Setting flag." << std::endl;
		g_isInWinlogonContext = true;
	}
	else
	{
		g_isInWinlogonContext = false; // Reset flag for other keys
	}

	// Call the original RegOpenKeyExA
	return g_pOriginalRegOpenKeyExA(hKey, lpSubKey, ulOptions, samDesired, phkResult);
}


// Hook function for RegQueryValueExA
LONG WINAPI MyRegQueryValueExAHook(
	HKEY hKey,
	LPCSTR lpValueName,
	LPDWORD lpReserved,
	LPDWORD lpType,
	LPBYTE lpData,
	LPDWORD lpcbData
)
{
	std::cout << "[Hook] RegQueryValueExA intercepted. Value: " << (lpValueName ? lpValueName : "") << std::endl;

	// Check if we are in the context of the Winlogon key
	if (g_isInWinlogonContext)
	{
		std::cout << "[Deception] Malware querying a value in Winlogon! Returning fake failure for: " << (lpValueName ? lpValueName : "") << std::endl;
		// Implement FakeFailure: Return an error code indicating the value was not found
		// For all values within the Winlogon context.
		if (lpcbData) * lpcbData = 0; // Indicate no data was returned
		if (lpType) * lpType = REG_NONE; // Indicate no type
		return ERROR_FILE_NOT_FOUND; // Return the same error for all values
	}

	// For any other registry query or if not in the sensitive key context,
	// call the original function to maintain normal functionality.
	return g_pOriginalRegQueryValueExA(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
}

// Existing Beep hook from previous example
DWORD gFreqOffset = 0; // from previous example
BOOL WINAPI myBeepHook(DWORD dwFreq, DWORD dwDuration)
{
	cout << "[+] BeepHook triggered!" << endl;
	cout << "Original Frequency: " << dwFreq << ", Duration: " << dwDuration << endl;
	return Beep(dwFreq + gFreqOffset, dwDuration);
}


extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
	cout << "[*] Injection started." << endl;

	// Beep Hook (from previous example)
	HOOK_TRACE_INFO hBeepHook = { NULL };
	FARPROC beepAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "Beep");
	cout << "Beep function address: " << (void*)beepAddr << endl;
	NTSTATUS beepResult = LhInstallHook(beepAddr, myBeepHook, nullptr, &hBeepHook);
	if (FAILED(beepResult))
	{
		wstring s(RtlGetLastErrorString());
		wcerr << L"Failed to install Beep hook: " << s << endl;
	}
	else
	{
		cout << "Beep hook installed successfully!" << endl;
	}


	// Registry Hooks
	HOOK_TRACE_INFO hRegOpenKeyExA = { NULL };
	HOOK_TRACE_INFO hRegQueryValueExA = { NULL };

	// Get addresses of original functions
	g_pOriginalRegOpenKeyExA = (FnRegOpenKeyExA)GetProcAddress(GetModuleHandle(TEXT("Advapi32")), "RegOpenKeyExA");
	g_pOriginalRegQueryValueExA = (FnRegQueryValueExA)GetProcAddress(GetModuleHandle(TEXT("Advapi32")), "RegQueryValueExA");

	if (g_pOriginalRegOpenKeyExA == nullptr || g_pOriginalRegQueryValueExA == nullptr)
	{
		wcerr << L"Failed to get addresses for registry API functions." << endl;
		return;
	}

	// Install hook for RegOpenKeyExA
	NTSTATUS resultOpen = LhInstallHook(
		g_pOriginalRegOpenKeyExA,
		MyRegOpenKeyExAHook,
		nullptr,
		&hRegOpenKeyExA
	);

	if (FAILED(resultOpen))
	{
		wstring s(RtlGetLastErrorString());
		wcerr << L"Failed to install RegOpenKeyExA hook: " << s << endl;
	}
	else
	{
		cout << "RegOpenKeyExA hook installed successfully!" << endl;
	}

	// Install hook for RegQueryValueExA
	NTSTATUS resultQuery = LhInstallHook(
		g_pOriginalRegQueryValueExA,
		MyRegQueryValueExAHook,
		nullptr,
		&hRegQueryValueExA
	);

	if (FAILED(resultQuery))
	{
		wstring s(RtlGetLastErrorString());
		wcerr << L"Failed to install RegQueryValueExA hook: " << s << endl;
	}
	else
	{
		cout << "RegQueryValueExA hook installed successfully!" << endl;
	}

	// Enable all hooks for all threads in the target process
	ULONG ACLEntries[3] = { 0 }; // Increased to accommodate 3 hooks if needed
	LhSetExclusiveACL(ACLEntries, 1, &hBeepHook); // ACL for Beep hook
	LhSetExclusiveACL(ACLEntries, 1, &hRegOpenKeyExA); // ACL for RegOpenKeyExA
	LhSetExclusiveACL(ACLEntries, 1, &hRegQueryValueExA); // ACL for RegQueryValueExA

	cout << "[*] Injection and hooks enabled." << endl;
}