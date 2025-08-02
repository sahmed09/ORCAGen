#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <easyhook.h>

#pragma comment(lib, "EasyHook32.lib")  // or EasyHook64.lib

using namespace std;

HOOK_TRACE_INFO hMoveFileHook = { nullptr };
HOOK_TRACE_INFO hMoveFileExHook = { nullptr };

// Suspicious ransomware extensions
vector<wstring> suspiciousExtensions = { L".locked", L".enc", L".REvil", L".encrypted" };

bool HasSuspiciousExtension(const wstring& filePath)
{
	for (const auto& ext : suspiciousExtensions)
	{
		if (filePath.length() >= ext.length() &&
			filePath.substr(filePath.length() - ext.length()) == ext)
		{
			return true;
		}
	}
	return false;
}

// Hooked MoveFileW
BOOL WINAPI myMoveFileWHook(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName)
{
	wcout << L"[Hook] MoveFileW called from: " << lpExistingFileName << L" -> " << lpNewFileName << endl;

	if (lpNewFileName && HasSuspiciousExtension(lpNewFileName))
	{
		wcout << L"[BLOCKED] Suspicious rename attempt blocked: " << lpNewFileName << endl;
		return TRUE; // Fake success to fool ransomware
	}

	return MoveFileW(lpExistingFileName, lpNewFileName);
}

// Hooked MoveFileExW
BOOL WINAPI myMoveFileExWHook(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, DWORD dwFlags)
{
	wcout << L"[Hook] MoveFileExW called from: " << lpExistingFileName << L" -> " << lpNewFileName << endl;

	if (lpNewFileName && HasSuspiciousExtension(lpNewFileName))
	{
		wcout << L"[BLOCKED] Suspicious MoveFileExW attempt blocked: " << lpNewFileName << endl;
		return TRUE; // Fake success
	}

	return MoveFileExW(lpExistingFileName, lpNewFileName, dwFlags);
}

// DLL entry point for EasyHook injection
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO* inRemoteInfo)
{
	cout << "[*] Ransomware Rename Hook DLL injected." << endl;

	FARPROC moveFileAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "MoveFileW");
	FARPROC moveFileExAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "MoveFileExW");

	if (SUCCEEDED(LhInstallHook(moveFileAddr, myMoveFileWHook, nullptr, &hMoveFileHook)))
	{
		cout << "[+] MoveFileW hook installed." << endl;
		ULONG acl[1] = { 0 };
		LhSetExclusiveACL(acl, 1, &hMoveFileHook);
	}

	if (SUCCEEDED(LhInstallHook(moveFileExAddr, myMoveFileExWHook, nullptr, &hMoveFileExHook)))
	{
		cout << "[+] MoveFileExW hook installed." << endl;
		ULONG acl[1] = { 0 };
		LhSetExclusiveACL(acl, 1, &hMoveFileExHook);
	}
}
