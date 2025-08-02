#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <easyhook.h>

#pragma comment(lib, "EasyHook32.lib") // Or EasyHook64.lib

using namespace std;

// Helper: Case-insensitive wstring comparison for extensions
bool EndsWithCaseInsensitive(const std::wstring& s, const std::wstring& suffix) {
	if (s.size() < suffix.size()) return false;
	return _wcsicmp(s.c_str() + (s.size() - suffix.size()), suffix.c_str()) == 0;
}

// Helper: Detect if the new file path has suspicious (ransomware) extensions
bool IsSuspiciousExtension(const std::wstring & newPath) {
	static std::vector<std::wstring> suspicious = {
		L".locked", L".enc", L".encrypted", L".REvil", L".LockBit", L".conti"
	};
	for (const auto& ext : suspicious) {
		if (EndsWithCaseInsensitive(newPath, ext)) return true;
		// Match with possible victim ID as an extra extension
		if (newPath.find(ext + L".") != std::wstring::npos) return true;
	}
	return false;
}

// Hooked MoveFileW
BOOL WINAPI myMoveFileWHook(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName) {
	std::wstring newName(lpNewFileName ? lpNewFileName : L"");
	if (IsSuspiciousExtension(newName)) {
		wcout << L"[RANSOMWARE BLOCKED] MoveFileW: " << lpExistingFileName << L" -> " << lpNewFileName << endl;
		// Return TRUE to fake success (malware thinks it worked, but it didn't)
		return TRUE;
	}
	return MoveFileW(lpExistingFileName, lpNewFileName);
}

// Hooked MoveFileExW
BOOL WINAPI myMoveFileExWHook(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, DWORD dwFlags) {
	std::wstring newName(lpNewFileName ? lpNewFileName : L"");
	if (IsSuspiciousExtension(newName)) {
		wcout << L"[RANSOMWARE BLOCKED] MoveFileExW: " << lpExistingFileName << L" -> " << lpNewFileName << endl;
		// Return TRUE to fake success (malware thinks it worked, but it didn't)
		return TRUE;
	}
	return MoveFileExW(lpExistingFileName, lpNewFileName, dwFlags);
}

// (Optional) Logging to file for forensics
void LogBlockedRename(const std::wstring & oldName, const std::wstring & newName) {
	FILE* logFile;
	_wfopen_s(&logFile, L"c:\\AntiRansomwareLog.txt", L"a+, ccs=UTF-8");
	if (logFile) {
		fwprintf(logFile, L"[BLOCKED] %s -> %s\n", oldName.c_str(), newName.c_str());
		fclose(logFile);
	}
}

// EasyHook entry point
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
	cout << "[*] Anti-Ransomware Rename Hook Injection started." << endl;

	HOOK_TRACE_INFO hMoveFileHook = { NULL };
	HOOK_TRACE_INFO hMoveFileExHook = { NULL };

	// Install MoveFileW hook
	FARPROC moveFileAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "MoveFileW");
	if (LhInstallHook(moveFileAddr, myMoveFileWHook, nullptr, &hMoveFileHook) != 0)
		wcout << L"[!] Failed to hook MoveFileW." << endl;

	// Install MoveFileExW hook
	FARPROC moveFileExAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "MoveFileExW");
	if (LhInstallHook(moveFileExAddr, myMoveFileExWHook, nullptr, &hMoveFileExHook) != 0)
		wcout << L"[!] Failed to hook MoveFileExW." << endl;

	// Enable all hooks for all threads in the process
	ULONG ACLEntries[1] = { 0 };
	LhSetExclusiveACL(ACLEntries, 1, &hMoveFileHook);
	LhSetExclusiveACL(ACLEntries, 1, &hMoveFileExHook);

	wcout << L"[*] Ransomware rename hooks are active.\n";
}
