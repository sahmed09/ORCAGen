#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <easyhook.h>

#pragma comment(lib, "EasyHook32.lib")  // or EasyHook64.lib

using namespace std;

// Suspicious ransomware-like extensions
const std::wstring suspiciousExtensions[] = { L".locked", L".enc", L".REvil" };

// Hooked: MoveFileW
BOOL WINAPI myMoveFileWHook(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName)
{
	wcout << L"[Hook] MoveFileW intercepted:\n";
	wcout << L"        From: " << lpExistingFileName << L"\n";
	wcout << L"        To  : " << lpNewFileName << endl;

	std::wstring newName(lpNewFileName);
	for (const auto& ext : suspiciousExtensions)
	{
		if (newName.find(ext) != std::wstring::npos)
		{
			wcerr << L"[BLOCKED] Suspicious rename attempt to: " << ext << endl;
			SetLastError(ERROR_ACCESS_DENIED);  // FakeFailure
			return FALSE;
		}
	}

	return MoveFileW(lpExistingFileName, lpNewFileName);
}

// Hooked: MoveFileExW
BOOL WINAPI myMoveFileExWHook(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, DWORD dwFlags)
{
	wcout << L"[Hook] MoveFileExW intercepted:\n";
	wcout << L"        From: " << lpExistingFileName << L"\n";
	wcout << L"        To  : " << lpNewFileName << endl;

	std::wstring newName(lpNewFileName);
	for (const auto& ext : suspiciousExtensions)
	{
		if (newName.find(ext) != std::wstring::npos)
		{
			wcerr << L"[BLOCKED] Suspicious extension in MoveFileExW: " << ext << endl;
			SetLastError(ERROR_ACCESS_DENIED);  // FakeFailure
			return FALSE;
		}
	}

	return MoveFileExW(lpExistingFileName, lpNewFileName, dwFlags);
}

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO* inRemoteInfo)
{
	cout << "[*] Injection started.\n";

	HOOK_TRACE_INFO hMoveFileHook = { NULL };
	HOOK_TRACE_INFO hMoveFileExHook = { NULL };

	// Resolve original API addresses
	FARPROC moveFileAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "MoveFileW");
	FARPROC moveFileExAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "MoveFileExW");

	// Install hooks
	NTSTATUS res1 = LhInstallHook(moveFileAddr, myMoveFileWHook, nullptr, &hMoveFileHook);
	NTSTATUS res2 = LhInstallHook(moveFileExAddr, myMoveFileExWHook, nullptr, &hMoveFileExHook);

	if (FAILED(res1) || FAILED(res2))
	{
		wcerr << L"[!] Failed to install one or more hooks.\n";
		wcerr << L"    Error: " << RtlGetLastErrorString() << endl;
		return;
	}

	// Enable hooks for all threads in the current process
	ULONG ACLEntries[1] = { 0 };
	LhSetExclusiveACL(ACLEntries, 1, &hMoveFileHook);
	LhSetExclusiveACL(ACLEntries, 1, &hMoveFileExHook);

	cout << "[+] Hooks installed successfully (MoveFileW and MoveFileExW).\n";
}
