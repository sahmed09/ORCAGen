#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <easyhook.h>
#include <random>
#include <algorithm> // Required for std::transform
#include <cctype>    // Required for std::tolower

#pragma comment(lib, "EasyHook32.lib") // or EasyHook64.lib depending on platform

using namespace std;

// Original function pointers
// These must be global or static to be accessible by the hook functions
// and to allow calling the real API.
typedef HANDLE(WINAPI* PFN_FindFirstFileW)(LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData);
PFN_FindFirstFileW g_pOriginalFindFirstFileW = NULL;

typedef BOOL(WINAPI* PFN_FindNextFileW)(HANDLE hFindFile, LPWIN32_FIND_DATAW lpFindFileData);
PFN_FindNextFileW g_pOriginalFindNextFileW = NULL;

// Random generator (for potential future deceptive randomness)
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dist(0, 99);

// --- Hooked Functions ---

// Hook for FindFirstFileW
HANDLE WINAPI Hook_FindFirstFileW(LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData)
{
	wcout << L"[Hook] FindFirstFileW intercepted for: " << lpFileName << endl;

	Sleep(100);

	// --- Deception Logic ---
	// Here, we check if the path being accessed by the malware contains keywords
	// that might indicate it's searching for sensitive files or directories.
	// We can also check if the current process is our target malware.

	std::wstring fileNameLower = lpFileName;
	std::transform(fileNameLower.begin(), fileNameLower.end(), fileNameLower.begin(),
		[](wchar_t c) { return static_cast<wchar_t>(std::tolower(c)); });

	// Define target directories/patterns for deception
	// This should match the targetScanPath from the malware PoC
	std::wstring deceptionTargetDir = L"c:\\users\\user\\documents\\malwareanalysis\\malwareevaluation-main\\clipboardlogger\\debug";
	std::transform(deceptionTargetDir.begin(), deceptionTargetDir.end(), deceptionTargetDir.begin(),
		[](wchar_t c) { return static_cast<wchar_t>(std::tolower(c)); });

	if (fileNameLower.find(deceptionTargetDir) != std::wstring::npos) {
		// If the malware is trying to scan our designated deceptive path
		wcout << L"[Deception] Intercepting file search in: " << lpFileName << L" - Returning fake failure." << endl;

		// Set last error to mimic "Access Denied" or "No more files"
		SetLastError(ERROR_ACCESS_DENIED); // Or ERROR_NO_MORE_FILES

		// Return INVALID_HANDLE_VALUE to indicate failure
		return INVALID_HANDLE_VALUE;
	}
	// --- End Deception Logic ---


	// If not a target for deception, call the original function
	return g_pOriginalFindFirstFileW(lpFileName, lpFindFileData);
}

// Hook for FindNextFileW
BOOL WINAPI Hook_FindNextFileW(HANDLE hFindFile, LPWIN32_FIND_DATAW lpFindFileData)
{
	wcout << L"[Hook] FindNextFileW intercepted." << endl;

	Sleep(100);

	// Call the original function
	return g_pOriginalFindNextFileW(hFindFile, lpFindFileData);
}


extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO* inRemoteInfo)
{
	cout << "[*] AntiClipboardLoggerHook: Injection started." << endl;

	HOOK_TRACE_INFO hFindFirstFileHook = { NULL };
	HOOK_TRACE_INFO hFindNextFileHook = { NULL };

	// Get addresses of the original functions
	FARPROC findFirstFileAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "FindFirstFileW");
	FARPROC findNextFileAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "FindNextFileW");

	if (findFirstFileAddr == NULL || findNextFileAddr == NULL) {
		wcerr << L"[ERROR] AntiClipboardLoggerHook: Could not find FindFirstFileW or FindNextFileW address." << endl;
		// Optionally, you might want to call LhUninstallHook and LhWaitForPendingRemovals here
		// if this is a critical error preventing proper functionality.
		return;
	}

	// Install hooks for FindFirstFileW
	// IMPORTANT: Pass the address of g_pOriginalFindFirstFileW to receive the original function pointer
	NTSTATUS result = LhInstallHook(
		findFirstFileAddr,
		Hook_FindFirstFileW,
		(PVOID*)& g_pOriginalFindFirstFileW, // This is the crucial change!
		&hFindFirstFileHook
	);

	if (FAILED(result)) {
		wstring s(RtlGetLastErrorString());
		wcerr << L"[ERROR] AntiClipboardLoggerHook: Failed to install FindFirstFileW hook: " << s << endl;
	}
	else {
		cout << "[*] AntiClipboardLoggerHook: FindFirstFileW hook installed successfully!" << endl;
	}

	// Install hooks for FindNextFileW
	// IMPORTANT: Pass the address of g_pOriginalFindNextFileW to receive the original function pointer
	result = LhInstallHook(
		findNextFileAddr,
		Hook_FindNextFileW,
		(PVOID*)& g_pOriginalFindNextFileW, // This is the crucial change!
		&hFindNextFileHook
	);

	if (FAILED(result)) {
		wstring s(RtlGetLastErrorString());
		wcerr << L"[ERROR] AntiClipboardLoggerHook: Failed to install FindNextFileW hook: " << s << endl;
	}
	else {
		cout << "[*] AntiClipboardLoggerHook: FindNextFileW hook installed successfully!" << endl;
	}

	// Enable all installed hooks for the current process
	ULONG ACLEntries[2]; // One for each hook
	ACLEntries[0] = 0; // Means "exclusive for current process"
	ACLEntries[1] = 0;

	// It's safer to use the return value from LhInstallHook for the ACLEntries
	// instead of relying on fixed indices if more hooks are added or removed.
	// But for two fixed hooks, this is acceptable.
	LhSetExclusiveACL(ACLEntries, 1, &hFindFirstFileHook);
	LhSetExclusiveACL(ACLEntries, 1, &hFindNextFileHook);

	cout << "[*] AntiClipboardLoggerHook: All hooks enabled." << endl;
}