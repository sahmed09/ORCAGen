#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <easyhook.h>

#pragma comment(lib, "EasyHook32.lib")  // Or EasyHook64.lib if building 64-bit

using namespace std;

// Store original function pointers
typedef HANDLE(WINAPI* FindFirstFileA_t)(LPCSTR, LPWIN32_FIND_DATAA);
typedef HANDLE(WINAPI* FindFirstFileW_t)(LPCWSTR, LPWIN32_FIND_DATAW);
typedef BOOL(WINAPI* FindNextFileA_t)(HANDLE, LPWIN32_FIND_DATAA);
typedef BOOL(WINAPI* FindNextFileW_t)(HANDLE, LPWIN32_FIND_DATAW);

FindFirstFileA_t TrueFindFirstFileA = nullptr;
FindFirstFileW_t TrueFindFirstFileW = nullptr;
FindNextFileA_t  TrueFindNextFileA = nullptr;
FindNextFileW_t  TrueFindNextFileW = nullptr;

// Deceptive FindFirstFileA hook
HANDLE WINAPI myFindFirstFileA(
	LPCSTR lpFileName,
	LPWIN32_FIND_DATAA lpFindFileData)
{
	cout << "[HOOK] FindFirstFileA intercepted for: " << (lpFileName ? lpFileName : "(null)") << endl;

	Sleep(50);

	// Simulate failure
	SetLastError(ERROR_ACCESS_DENIED); // or ERROR_FILE_NOT_FOUND
	cout << "[Deception] File search operation interrupted.\n";
	return INVALID_HANDLE_VALUE;
}

// Deceptive FindFirstFileW hook
HANDLE WINAPI myFindFirstFileW(
	LPCWSTR lpFileName,
	LPWIN32_FIND_DATAW lpFindFileData)
{
	wcout << L"[HOOK] FindFirstFileW intercepted for: " << (lpFileName ? lpFileName : L"(null)") << endl;

	Sleep(50);

	// Simulate failure
	SetLastError(ERROR_ACCESS_DENIED);
	wcout << L"[Deception] File search operation interrupted.\n";
	return INVALID_HANDLE_VALUE;
}

// Deceptive FindNextFileA hook
BOOL WINAPI myFindNextFileA(
	HANDLE hFindFile,
	LPWIN32_FIND_DATAA lpFindFileData)
{
	cout << "[HOOK] FindNextFileA intercepted." << endl;

	Sleep(50);

	// Simulate failure, no more files found
	SetLastError(ERROR_NO_MORE_FILES);
	cout << "[Deception] File search operation interrupted (next).\n";
	return FALSE;
}

// Deceptive FindNextFileW hook
BOOL WINAPI myFindNextFileW(
	HANDLE hFindFile,
	LPWIN32_FIND_DATAW lpFindFileData)
{
	wcout << L"[HOOK] FindNextFileW intercepted." << endl;

	Sleep(50);

	SetLastError(ERROR_NO_MORE_FILES);
	wcout << L"[Deception] File search operation interrupted (next).\n";
	return FALSE;
}

// DLL Entry Point for EasyHook Injection
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
	cout << "[*] AntiFileScanDeceptionHook Injection started." << endl;

	HOOK_TRACE_INFO hFindFirstA = { NULL };
	HOOK_TRACE_INFO hFindFirstW = { NULL };
	HOOK_TRACE_INFO hFindNextA = { NULL };
	HOOK_TRACE_INFO hFindNextW = { NULL };

	// Resolve addresses of the original APIs
	TrueFindFirstFileA = (FindFirstFileA_t)GetProcAddress(GetModuleHandleA("kernel32.dll"), "FindFirstFileA");
	TrueFindFirstFileW = (FindFirstFileW_t)GetProcAddress(GetModuleHandleA("kernel32.dll"), "FindFirstFileW");
	TrueFindNextFileA = (FindNextFileA_t)GetProcAddress(GetModuleHandleA("kernel32.dll"), "FindNextFileA");
	TrueFindNextFileW = (FindNextFileW_t)GetProcAddress(GetModuleHandleA("kernel32.dll"), "FindNextFileW");

	// Install hooks
	NTSTATUS res1 = LhInstallHook(TrueFindFirstFileA, myFindFirstFileA, nullptr, &hFindFirstA);
	NTSTATUS res2 = LhInstallHook(TrueFindFirstFileW, myFindFirstFileW, nullptr, &hFindFirstW);
	NTSTATUS res3 = LhInstallHook(TrueFindNextFileA, myFindNextFileA, nullptr, &hFindNextA);
	NTSTATUS res4 = LhInstallHook(TrueFindNextFileW, myFindNextFileW, nullptr, &hFindNextW);

	if (FAILED(res1) || FAILED(res2) || FAILED(res3) || FAILED(res4)) {
		wstring s(RtlGetLastErrorString());
		wcerr << L"Failed to install one or more hooks: " << s << endl;
		return;
	}
	else {
		cout << "All file search hooks installed successfully!" << endl;
	}

	ULONG ACLEntries[1] = { 0 };
	LhSetExclusiveACL(ACLEntries, 1, &hFindFirstA);
	LhSetExclusiveACL(ACLEntries, 1, &hFindFirstW);
	LhSetExclusiveACL(ACLEntries, 1, &hFindNextA);
	LhSetExclusiveACL(ACLEntries, 1, &hFindNextW);
}
