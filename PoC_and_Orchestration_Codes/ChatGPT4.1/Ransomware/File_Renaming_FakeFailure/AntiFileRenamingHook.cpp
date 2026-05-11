#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <easyhook.h>
#include <vector>
#include <regex>

#pragma comment(lib, "EasyHook32.lib") // Use EasyHook64.lib if 64-bit

using namespace std;

// Helper: Check for ransomware extensions in filename
bool HasSuspiciousExtension(const std::wstring& fileName) {
	std::vector<std::wstring> exts = { L".locked", L".enc", L".REvil", L".encrypted" };
	for (const auto& ext : exts) {
		if (fileName.length() >= ext.length() &&
			fileName.substr(fileName.length() - ext.length()) == ext) {
			return true;
		}
	}
	// Regex for base64-like victim ID in extension: .[A-Za-z0-9+/=]+.(locked|enc|REvil|encrypted)
	std::wregex pat(LR"(\.[A-Za-z0-9+/=]+\.(locked|enc|REvil|encrypted)$)", std::regex_constants::icase);
	return std::regex_search(fileName, pat);
}

// --- Hook for MoveFileW ---
BOOL WINAPI myMoveFileWHook(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName) {
	if (lpNewFileName && HasSuspiciousExtension(lpNewFileName)) {
		wcout << L"[ActiveDefense] Blocked suspicious file rename (MoveFileW): " << lpExistingFileName
			<< L" -> " << lpNewFileName << endl;
		SetLastError(ERROR_ACCESS_DENIED); // Simulate failure
		return FALSE;
	}
	// Legitimate rename; pass through
	return MoveFileW(lpExistingFileName, lpNewFileName);
}

// --- Hook for MoveFileExW ---
BOOL WINAPI myMoveFileExWHook(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, DWORD dwFlags) {
	if (lpNewFileName && HasSuspiciousExtension(lpNewFileName)) {
		wcout << L"[ActiveDefense] Blocked suspicious file rename (MoveFileExW): " << lpExistingFileName
			<< L" -> " << lpNewFileName << endl;
		SetLastError(ERROR_ACCESS_DENIED);
		return FALSE;
	}
	return MoveFileExW(lpExistingFileName, lpNewFileName, dwFlags);
}

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO* inRemoteInfo) {
	cout << "[*] Injection started (AntiRansomwareFileRenameHook)." << endl;

	HOOK_TRACE_INFO hMoveFileWHook = { NULL };
	HOOK_TRACE_INFO hMoveFileExWHook = { NULL };

	FARPROC moveFileAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "MoveFileW");
	FARPROC moveFileExAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "MoveFileExW");

	// Install hooks
	if (SUCCEEDED(LhInstallHook(moveFileAddr, myMoveFileWHook, nullptr, &hMoveFileWHook))) {
		cout << "[+] MoveFileW hook installed." << endl;
	}
	if (SUCCEEDED(LhInstallHook(moveFileExAddr, myMoveFileExWHook, nullptr, &hMoveFileExWHook))) {
		cout << "[+] MoveFileExW hook installed." << endl;
	}

	// Enable all hooks for all threads in the process
	ULONG ACLEntries[1] = { 0 };
	LhSetExclusiveACL(ACLEntries, 1, &hMoveFileWHook);
	LhSetExclusiveACL(ACLEntries, 1, &hMoveFileExWHook);
}
