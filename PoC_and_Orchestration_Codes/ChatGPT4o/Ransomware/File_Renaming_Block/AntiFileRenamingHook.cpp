#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <easyhook.h>
#include <iostream>
#include <string>
#include <vector>

#pragma comment(lib, "EasyHook32.lib") // or EasyHook64.lib

using namespace std;

// List of known ransomware extensions
const std::vector<std::wstring> suspiciousExtensions = {
	L".locked", L".enc", L".REvil"
};

// Utility function to check if path ends with suspicious extension
bool HasSuspiciousExtension(const std::wstring& filePath)
{
	for (const auto& ext : suspiciousExtensions)
	{
		if (filePath.size() >= ext.size() &&
			filePath.compare(filePath.size() - ext.size(), ext.size(), ext) == 0)
		{
			return true;
		}
	}
	return false;
}

// Hooked MoveFileW
BOOL WINAPI myMoveFileWHook(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName)
{
	wcout << L"[Hook] MoveFileW: " << lpExistingFileName << L" -> " << lpNewFileName << endl;

	if (lpNewFileName && HasSuspiciousExtension(lpNewFileName))
	{
		wcout << L"[Blocked] Suspicious rename attempt blocked: " << lpNewFileName << endl;
		SetLastError(ERROR_ACCESS_DENIED);
		return FALSE;
	}

	return MoveFileW(lpExistingFileName, lpNewFileName);
}

// Hooked MoveFileExW
BOOL WINAPI myMoveFileExWHook(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, DWORD dwFlags)
{
	wcout << L"[Hook] MoveFileExW: " << lpExistingFileName << L" -> " << lpNewFileName << endl;

	if (lpNewFileName && HasSuspiciousExtension(lpNewFileName))
	{
		wcout << L"[Blocked] Suspicious rename attempt blocked: " << lpNewFileName << endl;
		SetLastError(ERROR_ACCESS_DENIED);
		return FALSE;
	}

	return MoveFileExW(lpExistingFileName, lpNewFileName, dwFlags);
}

// Entry point called after injection
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO* inRemoteInfo)
{
	wcout << L"[*] Ransomware defense hook activated!" << endl;

	HOOK_TRACE_INFO hMoveFileHook = { NULL };
	HOOK_TRACE_INFO hMoveFileExHook = { NULL };

	// Install hooks
	FARPROC moveFileAddr = GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "MoveFileW");
	FARPROC moveFileExAddr = GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "MoveFileExW");

	if (FAILED(LhInstallHook(moveFileAddr, myMoveFileWHook, nullptr, &hMoveFileHook)))
	{
		wcerr << L"[-] Failed to hook MoveFileW" << endl;
	}
	else
	{
		wcout << L"[+] MoveFileW hooked successfully" << endl;
	}

	if (FAILED(LhInstallHook(moveFileExAddr, myMoveFileExWHook, nullptr, &hMoveFileExHook)))
	{
		wcerr << L"[-] Failed to hook MoveFileExW" << endl;
	}
	else
	{
		wcout << L"[+] MoveFileExW hooked successfully" << endl;
	}

	// Allow hooks only in the current thread (process-wide)
	ULONG ACLEntries[1] = { 0 };
	LhSetExclusiveACL(ACLEntries, 1, &hMoveFileHook);
	LhSetExclusiveACL(ACLEntries, 1, &hMoveFileExHook);
}
