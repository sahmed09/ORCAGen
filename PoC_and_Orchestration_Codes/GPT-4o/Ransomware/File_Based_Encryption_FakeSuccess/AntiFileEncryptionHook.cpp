#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <easyhook.h>
#include <iostream>
#include <string>
#include <vector>

#pragma comment(lib, "EasyHook32.lib")

using namespace std;

std::vector<std::wstring> protectedExtensions = { L".docx", L".xlsx", L".db", L".txt" };

// Helper: check if file path ends with one of the protected extensions
bool isProtectedFile(const std::wstring& path)
{
	for (const auto& ext : protectedExtensions)
	{
		if (path.size() >= ext.size() &&
			path.compare(path.size() - ext.size(), ext.size(), ext) == 0)
		{
			return true;
		}
	}
	return false;
}

// ------------------ Hook: CreateFileW ------------------
HANDLE WINAPI myCreateFileWHook(
	LPCWSTR lpFileName,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes,
	HANDLE hTemplateFile)
{
	wcout << L"[Hook] CreateFileW intercepted: " << lpFileName << endl;
	return CreateFileW(lpFileName, dwDesiredAccess, dwShareMode,
		lpSecurityAttributes, dwCreationDisposition,
		dwFlagsAndAttributes, hTemplateFile);
}

// ------------------ Hook: WriteFile ------------------
BOOL WINAPI myWriteFileHook(
	HANDLE hFile,
	LPCVOID lpBuffer,
	DWORD nNumberOfBytesToWrite,
	LPDWORD lpNumberOfBytesWritten,
	LPOVERLAPPED lpOverlapped)
{
	wchar_t filePath[MAX_PATH] = { 0 };
	GetFinalPathNameByHandleW(hFile, filePath, MAX_PATH, FILE_NAME_NORMALIZED);

	if (isProtectedFile(filePath))
	{
		wcout << L"[Deception] WriteFile blocked for: " << filePath << endl;
		if (lpNumberOfBytesWritten)* lpNumberOfBytesWritten = nNumberOfBytesToWrite;
		return TRUE; // Fake success
	}

	return WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
}

// ------------------ Hook: SetFilePointer ------------------
DWORD WINAPI mySetFilePointerHook(
	HANDLE hFile,
	LONG lDistanceToMove,
	PLONG lpDistanceToMoveHigh,
	DWORD dwMoveMethod)
{
	wchar_t filePath[MAX_PATH] = { 0 };
	GetFinalPathNameByHandleW(hFile, filePath, MAX_PATH, FILE_NAME_NORMALIZED);

	if (isProtectedFile(filePath))
	{
		wcout << L"[Deception] SetFilePointer spoofed for: " << filePath << endl;
		return 0; // Fake success (start of file)
	}

	return SetFilePointer(hFile, lDistanceToMove, lpDistanceToMoveHigh, dwMoveMethod);
}

// ------------------ Hook: FlushFileBuffers ------------------
BOOL WINAPI myFlushFileBuffersHook(HANDLE hFile)
{
	wchar_t filePath[MAX_PATH] = { 0 };
	GetFinalPathNameByHandleW(hFile, filePath, MAX_PATH, FILE_NAME_NORMALIZED);

	if (isProtectedFile(filePath))
	{
		wcout << L"[Deception] FlushFileBuffers spoofed for: " << filePath << endl;
		return TRUE; // Pretend buffers are flushed
	}

	return FlushFileBuffers(hFile);
}

// ------------------ Entry Point ------------------
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO* inRemoteInfo)
{
	wcout << L"[*] Anti-Ransomware Injection Started\n";

	HOOK_TRACE_INFO hCreateHook = { NULL };
	HOOK_TRACE_INFO hWriteHook = { NULL };
	HOOK_TRACE_INFO hSetPointerHook = { NULL };
	HOOK_TRACE_INFO hFlushHook = { NULL };

	// Resolve API addresses
	FARPROC createFileAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "CreateFileW");
	FARPROC writeFileAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "WriteFile");
	FARPROC setPointerAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "SetFilePointer");
	FARPROC flushAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "FlushFileBuffers");

	// Install hooks
	LhInstallHook(createFileAddr, myCreateFileWHook, nullptr, &hCreateHook);
	LhInstallHook(writeFileAddr, myWriteFileHook, nullptr, &hWriteHook);
	LhInstallHook(setPointerAddr, mySetFilePointerHook, nullptr, &hSetPointerHook);
	LhInstallHook(flushAddr, myFlushFileBuffersHook, nullptr, &hFlushHook);

	// Enable all for current process
	ULONG ACLEntries[1] = { 0 };
	LhSetExclusiveACL(ACLEntries, 1, &hCreateHook);
	LhSetExclusiveACL(ACLEntries, 1, &hWriteHook);
	LhSetExclusiveACL(ACLEntries, 1, &hSetPointerHook);
	LhSetExclusiveACL(ACLEntries, 1, &hFlushHook);
}
