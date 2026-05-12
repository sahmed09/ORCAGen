#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>     // Required for std::vector
#include <algorithm>  // Required for std::transform
#include <cctype>     // Required for ::tolower, but for wchar_t use <wctype.h>
#include <wctype.h>   // Required for ::towlower
#include <easyhook.h>

#pragma comment(lib, "EasyHook32.lib") // or EasyHook64.lib depending on platform

using namespace std;

// Original function pointers
// Use these as the first argument in LhInstallHook, and then cast to the actual function pointer type.
typedef BOOL(WINAPI* PFN_MoveFileExW)(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, DWORD dwFlags);
// Store the original address obtained from GetProcAddress, before hooking
PFN_MoveFileExW TrueMoveFileExW = NULL;

// List of suspicious ransomware extensions
const std::vector<std::wstring> suspiciousExtensions = {
	L".locked",
	L".enc",
	L".REvil",
	L".encrypted",
	L".ransom",
	L".crypt",
	L".xyz" // Add more as needed
};

// Hook function for MoveFileExW
BOOL WINAPI Detour_MoveFileExW(
	LPCWSTR lpExistingFileName,
	LPCWSTR lpNewFileName,
	DWORD dwFlags)
{
	wcout << L"[Hook] MoveFileExW intercepted!" << endl;
	wcout << L"  Original: " << lpExistingFileName << endl;
	wcout << L"  New:      " << lpNewFileName << endl;

	// --- Ransomware Detection Logic ---
	if (lpNewFileName != NULL)
	{
		std::wstring newFileNameStr(lpNewFileName);
		bool isSuspicious = false;

		// Convert newFileNameStr to lowercase for case-insensitive comparison
		// Using ::towlower from global namespace (via <wctype.h>)
		std::transform(newFileNameStr.begin(), newFileNameStr.end(), newFileNameStr.begin(),
			[](wchar_t c) { return ::towlower(c); }); // Corrected: removed std:: before towlower

		for (const auto& ext : suspiciousExtensions)
		{
			// Check if the new file name ends with a suspicious extension
			if (newFileNameStr.length() >= ext.length() &&
				newFileNameStr.compare(newFileNameStr.length() - ext.length(), ext.length(), ext) == 0)
			{
				isSuspicious = true;
				break;
			}
		}

		if (isSuspicious)
		{
			wcout << L"[RANSOMWARE DETECTED] Blocking suspicious file rename attempt!" << endl;
			SetLastError(ERROR_ACCESS_DENIED); // Set an error to indicate denial
			return FALSE; // Block the operation
		}
	}
	// --- End Ransomware Detection Logic ---

	// If not suspicious, call the original MoveFileExW function
	wcout << L"[Legitimate] Allowing file rename." << endl;
	// Call the stored original function pointer
	return TrueMoveFileExW(lpExistingFileName, lpNewFileName, dwFlags);
}


extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO* inRemoteInfo)
{
	cout << "[*] Injection started for AntiClipboardLoggerHook." << endl;

	HOOK_TRACE_INFO hMoveFileExHook = { NULL };

	// Get the address of the original MoveFileExW function
	FARPROC moveFileExAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "MoveFileExW");
	if (moveFileExAddr == NULL)
	{
		wcerr << L"Failed to get address for MoveFileExW. Error: " << GetLastError() << endl;
		return;
	}
	wcout << L"MoveFileExW function address: " << (void*)moveFileExAddr << endl;

	// IMPORTANT: Cast and store the original function pointer BEFORE installing the hook.
	// EasyHook's LhInstallHook often redirects the initial pointer.
	TrueMoveFileExW = (PFN_MoveFileExW)moveFileExAddr;

	// Install the hook for MoveFileExW
	NTSTATUS result = LhInstallHook(
		moveFileExAddr,          // Address of the original function
		Detour_MoveFileExW,      // Our hook function
		NULL,                    // No callback data
		&hMoveFileExHook         // OUT: Handle to the hook
	);

	if (FAILED(result))
	{
		wstring s(RtlGetLastErrorString());
		wcerr << L"Failed to install MoveFileExW hook: " << s << endl;
		return;
	}
	else
	{
		cout << "MoveFileExW hook installed successfully!" << endl;
	}

	// Apply the hook globally to all threads in the target process
	ULONG ACLEntries[1] = { 0 };
	LhSetExclusiveACL(ACLEntries, 1, &hMoveFileExHook);

	// Placeholder for other hooks (Beep, GetAsyncKeyState) if you re-add them.
	// Remember to get their original addresses and store them similarly.
	/*
	HOOK_TRACE_INFO hBeepHook = { NULL };
	FARPROC beepAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "Beep");
	// PFN_Beep TrueBeep = (PFN_Beep)beepAddr; // Define PFN_Beep if needed
	LhInstallHook(beepAddr, myBeepHook, nullptr, &hBeepHook);
	LhSetExclusiveACL(ACLEntries, 1, &hBeepHook);
	*/

	cout << "[*] AntiClipboardLoggerHook injection complete and hooks activated." << endl;
}