#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <easyhook.h>
#include <iostream>
#include <string>
#include <vector>

#pragma comment(lib, "EasyHook32.lib") // Change to EasyHook64.lib for 64-bit builds

using namespace std;

// Track sensitive extensions
vector<wstring> sensitiveExtensions = { L".docx", L".xlsx", L".db", L".txt" };

// Track file handles blocked by our defense
vector<HANDLE> blockedHandles;

// Checks if the file path ends with a sensitive extension
bool hasSensitiveExtension(const wstring& path)
{
	for (const auto& ext : sensitiveExtensions)
	{
		if (path.length() >= ext.length() &&
			path.compare(path.length() - ext.length(), ext.length(), ext) == 0)
		{
			return true;
		}
	}
}

// ---------------- Hook Implementations ----------------

// Hooked CreateFileW
HANDLE WINAPI myCreateFileWHook(
	LPCWSTR lpFileName,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes,
	HANDLE hTemplateFile)
{
	wstring filename(lpFileName);
	wcout << L"[Hook] CreateFileW intercepted: " << filename << endl;

	// Block write access to sensitive files
	if ((dwDesiredAccess & (GENERIC_WRITE | GENERIC_ALL)) && hasSensitiveExtension(filename))
	{
		wcout << L"[FakeFailure] Blocking access to: " << filename << endl;
		SetLastError(ERROR_ACCESS_DENIED);
		return INVALID_HANDLE_VALUE;
	}

	// Legit access — allow
	return CreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
		dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

// Hooked WriteFile
BOOL WINAPI myWriteFileHook(
	HANDLE hFile,
	LPCVOID lpBuffer,
	DWORD nNumberOfBytesToWrite,
	LPDWORD lpNumberOfBytesWritten,
	LPOVERLAPPED lpOverlapped)
{
	// Check if this handle is already flagged
	if (std::find(blockedHandles.begin(), blockedHandles.end(), hFile) != blockedHandles.end())
	{
		wcout << L"[FakeFailure] WriteFile blocked for known handle!" << endl;
		SetLastError(ERROR_WRITE_FAULT);
		if (lpNumberOfBytesWritten)* lpNumberOfBytesWritten = 0;
		return FALSE;
	}

	// Attempt real write
	BOOL result = WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);

	return result;
}

// ---------------- EasyHook Injection Entry Point ----------------

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO* inRemoteInfo)
{
	wcout << L"[*] Anti-Ransomware Hook DLL Injected." << endl;

	// Hook CreateFileW
	HOOK_TRACE_INFO hCreateFileHook = { NULL };
	FARPROC createFileAddr = GetProcAddress(GetModuleHandleA("kernel32.dll"), "CreateFileW");

	if (SUCCEEDED(LhInstallHook(createFileAddr, myCreateFileWHook, nullptr, &hCreateFileHook)))
	{
		ULONG acl[1] = { 0 };
		LhSetExclusiveACL(acl, 1, &hCreateFileHook);
		wcout << L"[+] Hook installed: CreateFileW" << endl;
	}
	else
	{
		wcerr << L"[!] Failed to hook CreateFileW." << endl;
	}

	// Hook WriteFile
	HOOK_TRACE_INFO hWriteFileHook = { NULL };
	FARPROC writeFileAddr = GetProcAddress(GetModuleHandleA("kernel32.dll"), "WriteFile");

	if (SUCCEEDED(LhInstallHook(writeFileAddr, myWriteFileHook, nullptr, &hWriteFileHook)))
	{
		ULONG acl[1] = { 0 };
		LhSetExclusiveACL(acl, 1, &hWriteFileHook);
		wcout << L"[+] Hook installed: WriteFile" << endl;
	}
	else
	{
		wcerr << L"[!] Failed to hook WriteFile." << endl;
	}

	wcout << L"[*] Hooks installed. Monitoring ransomware behavior..." << endl;
}
