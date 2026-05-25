#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <easyhook.h>

#pragma comment(lib, "EasyHook32.lib")  // Use EasyHook64.lib for 64-bit processes

using namespace std;

// Global handle for hooks
HOOK_TRACE_INFO hFindFirstFileHook = { NULL };
HOOK_TRACE_INFO hFindNextFileHook = { NULL };

// Function pointer types
typedef HANDLE(WINAPI* PFN_FindFirstFileW)(LPCWSTR, LPWIN32_FIND_DATAW);
typedef BOOL(WINAPI* PFN_FindNextFileW)(HANDLE, LPWIN32_FIND_DATAW);

// Original function pointers
PFN_FindFirstFileW TrueFindFirstFileW = FindFirstFileW;
PFN_FindNextFileW TrueFindNextFileW = FindNextFileW;

// Deceptive FindFirstFileW
HANDLE WINAPI myFindFirstFileWHook(LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData)
{
	wcout << L"[Hook] FindFirstFileW intercepted for: " << lpFileName << endl;

	Sleep(100);

	// Return INVALID_HANDLE_VALUE to simulate no matching files
	SetLastError(ERROR_FILE_NOT_FOUND);
	wcout << L"[Deception] File search blocked - simulated failure." << endl;
	return INVALID_HANDLE_VALUE;
}

// Deceptive FindNextFileW
BOOL WINAPI myFindNextFileWHook(HANDLE hFindFile, LPWIN32_FIND_DATAW lpFindFileData)
{
	wcout << L"[Hook] FindNextFileW intercepted." << endl;

	Sleep(100);

	// Always return FALSE to indicate no more files
	SetLastError(ERROR_NO_MORE_FILES);
	return FALSE;
}

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO* inRemoteInfo)
{
	cout << "[*] Injection started: AntiFileScanHook" << endl;

	// Install hooks
	FARPROC findFirstAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "FindFirstFileW");
	FARPROC findNextAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "FindNextFileW");

	if (FAILED(LhInstallHook(findFirstAddr, myFindFirstFileWHook, NULL, &hFindFirstFileHook))) {
		wcerr << L"[!] Failed to hook FindFirstFileW" << endl;
		return;
	}
	if (FAILED(LhInstallHook(findNextAddr, myFindNextFileWHook, NULL, &hFindNextFileHook))) {
		wcerr << L"[!] Failed to hook FindNextFileW" << endl;
		return;
	}

	cout << "[+] API hooks for file scan deception installed successfully." << endl;

	// Restrict hooks to the current process (malware target only)
	ULONG ACLEntries[1] = { 0 };
	LhSetExclusiveACL(ACLEntries, 1, &hFindFirstFileHook);
	LhSetExclusiveACL(ACLEntries, 1, &hFindNextFileHook);
}
