#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <easyhook.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#pragma comment(lib, "EasyHook32.lib") // or EasyHook64.lib

using namespace std;

// Suspicious ransomware extensions
std::vector<std::wstring> g_suspicious_exts = {
	L".locked", L".enc", L".encrypted", L".REvil"
};

// Utility: Case-insensitive ends-with for wide strings
bool has_suspicious_extension(const std::wstring& filename) {
	for (const auto& ext : g_suspicious_exts) {
		if (filename.length() >= ext.length() &&
			_wcsicmp(filename.c_str() + (filename.length() - ext.length()), ext.c_str()) == 0) {
			return true;
		}
	}
	return false;
}

// Original function pointers
typedef BOOL(WINAPI* MoveFileW_t)(LPCWSTR, LPCWSTR);
MoveFileW_t Real_MoveFileW = MoveFileW;

typedef BOOL(WINAPI* MoveFileExW_t)(LPCWSTR, LPCWSTR, DWORD);
MoveFileExW_t Real_MoveFileExW = MoveFileExW;

// MoveFileW hook
BOOL WINAPI myMoveFileWHook(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName) {
	std::wstring newName = lpNewFileName ? lpNewFileName : L"";
	if (has_suspicious_extension(newName)) {
		wcout << L"[BLOCKED][Ransomware Defense] Attempt to rename file to suspicious extension: " << newName << endl;
		SetLastError(ERROR_ACCESS_DENIED);
		return FALSE;
	}
	return Real_MoveFileW(lpExistingFileName, lpNewFileName);
}

// MoveFileExW hook
BOOL WINAPI myMoveFileExWHook(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, DWORD dwFlags) {
	std::wstring newName = lpNewFileName ? lpNewFileName : L"";
	if (has_suspicious_extension(newName)) {
		wcout << L"[BLOCKED][Ransomware Defense] Attempt to rename file to suspicious extension: " << newName << endl;
		SetLastError(ERROR_ACCESS_DENIED);
		return FALSE;
	}
	return Real_MoveFileExW(lpExistingFileName, lpNewFileName, dwFlags);
}

// DLL Entry Point for EasyHook Injection
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO* inRemoteInfo) {
	cout << "[*] Anti-Ransomware Rename Hook DLL injected." << endl;

	HOOK_TRACE_INFO hMoveFileHook = { NULL };
	HOOK_TRACE_INFO hMoveFileExHook = { NULL };

	// Install MoveFileW hook
	FARPROC moveFileAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "MoveFileW");
	LhInstallHook(moveFileAddr, myMoveFileWHook, nullptr, &hMoveFileHook);

	// Install MoveFileExW hook
	FARPROC moveFileExAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "MoveFileExW");
	LhInstallHook(moveFileExAddr, myMoveFileExWHook, nullptr, &hMoveFileExHook);

	// Enable hooks for ALL threads in process
	ULONG ACLEntries[1] = { 0 };
	LhSetExclusiveACL(ACLEntries, 1, &hMoveFileHook);
	LhSetExclusiveACL(ACLEntries, 1, &hMoveFileExHook);

	cout << "[*] Ransomware file renaming defense hooks enabled." << endl;
}
