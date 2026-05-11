#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <easyhook.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#pragma comment(lib, "EasyHook32.lib") // Use EasyHook64.lib for 64-bit

using namespace std;

// === Configuration: Set these to match your PoC ===
const std::wstring TARGET_DIR = L"C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug\\Files\\";
const std::vector<std::wstring> TARGET_EXTENSIONS = { L".docx", L".xlsx", L".db", L".txt" };

// === Utility: Wide/narrow string helpers ===
bool hasTargetExtension(const std::wstring& filename) {
	for (const auto& ext : TARGET_EXTENSIONS) {
		if (filename.length() >= ext.length() &&
			filename.compare(filename.length() - ext.length(), ext.length(), ext) == 0) {
			return true;
		}
	}
	return false;
}

bool isTargetedFile(const std::wstring& filepath) {
	// Check directory and extension
	if (filepath.find(TARGET_DIR) == 0 && hasTargetExtension(filepath))
		return true;
	return false;
}

// === Original function pointers ===
typedef HANDLE(WINAPI * CreateFileW_t)(
	LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
CreateFileW_t TrueCreateFileW = CreateFileW;

typedef HANDLE(WINAPI * CreateFileA_t)(
	LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
CreateFileA_t TrueCreateFileA = CreateFileA;

typedef BOOL(WINAPI * WriteFile_t)(
	HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED);
WriteFile_t TrueWriteFile = WriteFile;

typedef DWORD(WINAPI * SetFilePointer_t)(
	HANDLE, LONG, PLONG, DWORD);
SetFilePointer_t TrueSetFilePointer = SetFilePointer;

typedef BOOL(WINAPI * FlushFileBuffers_t)(HANDLE);
FlushFileBuffers_t TrueFlushFileBuffers = FlushFileBuffers;

// === Interceptors ===
HANDLE WINAPI myCreateFileWHook(
	LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
	std::wstring fname(lpFileName ? lpFileName : L"");
	if (isTargetedFile(fname)) {
		wcout << L"[BLOCKED] Ransomware attempt: CreateFileW: " << fname << endl;
		SetLastError(ERROR_ACCESS_DENIED);
		return INVALID_HANDLE_VALUE; // Fake failure
	}
	return TrueCreateFileW(lpFileName, dwDesiredAccess, dwShareMode,
		lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

HANDLE WINAPI myCreateFileAHook(
	LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
	std::wstring fname;
	if (lpFileName) {
		int len = MultiByteToWideChar(CP_ACP, 0, lpFileName, -1, NULL, 0);
		if (len > 0) {
			std::vector<wchar_t> buf(len);
			MultiByteToWideChar(CP_ACP, 0, lpFileName, -1, buf.data(), len);
			fname.assign(buf.data());
		}
	}
	if (isTargetedFile(fname)) {
		wcout << L"[BLOCKED] Ransomware attempt: CreateFileA: " << fname << endl;
		SetLastError(ERROR_ACCESS_DENIED);
		return INVALID_HANDLE_VALUE;
	}
	return TrueCreateFileA(lpFileName, dwDesiredAccess, dwShareMode,
		lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

BOOL WINAPI myWriteFileHook(
	HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
	LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped)
{
	wchar_t pathBuf[MAX_PATH];
	if (GetFinalPathNameByHandleW(hFile, pathBuf, MAX_PATH, 0) > 0) {
		std::wstring fname(pathBuf);
		// Remove "\\?\" prefix if present
		if (fname.substr(0, 4) == L"\\\\?\\")
			fname = fname.substr(4);
		if (isTargetedFile(fname)) {
			wcout << L"[BLOCKED] Ransomware attempt: WriteFile: " << fname << endl;
			SetLastError(ERROR_ACCESS_DENIED);
			if (lpNumberOfBytesWritten)* lpNumberOfBytesWritten = 0;
			return FALSE; // Fake failure
		}
	}
	return TrueWriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
}

DWORD WINAPI mySetFilePointerHook(
	HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod)
{
	wchar_t pathBuf[MAX_PATH];
	if (GetFinalPathNameByHandleW(hFile, pathBuf, MAX_PATH, 0) > 0) {
		std::wstring fname(pathBuf);
		if (fname.substr(0, 4) == L"\\\\?\\")
			fname = fname.substr(4);
		if (isTargetedFile(fname)) {
			wcout << L"[BLOCKED] Ransomware attempt: SetFilePointer: " << fname << endl;
			SetLastError(ERROR_ACCESS_DENIED);
			return INVALID_SET_FILE_POINTER; // Fake failure
		}
	}
	return TrueSetFilePointer(hFile, lDistanceToMove, lpDistanceToMoveHigh, dwMoveMethod);
}

BOOL WINAPI myFlushFileBuffersHook(HANDLE hFile)
{
	wchar_t pathBuf[MAX_PATH];
	if (GetFinalPathNameByHandleW(hFile, pathBuf, MAX_PATH, 0) > 0) {
		std::wstring fname(pathBuf);
		if (fname.substr(0, 4) == L"\\\\?\\")
			fname = fname.substr(4);
		if (isTargetedFile(fname)) {
			wcout << L"[BLOCKED] Ransomware attempt: FlushFileBuffers: " << fname << endl;
			SetLastError(ERROR_ACCESS_DENIED);
			return FALSE; // Fake failure
		}
	}
	return TrueFlushFileBuffers(hFile);
}

// === Hook Registration ===
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
	cout << "[*] Anti-ransomware injection started." << endl;

	HOOK_TRACE_INFO hCreateFileWHook = { NULL }, hCreateFileAHook = { NULL }, hWriteFileHook = { NULL }, hSetFilePointerHook = { NULL }, hFlushFileBuffersHook = { NULL };

	// Install hooks
	LhInstallHook(GetProcAddress(GetModuleHandle(TEXT("kernel32")), "CreateFileW"), myCreateFileWHook, nullptr, &hCreateFileWHook);
	LhInstallHook(GetProcAddress(GetModuleHandle(TEXT("kernel32")), "CreateFileA"), myCreateFileAHook, nullptr, &hCreateFileAHook);
	LhInstallHook(GetProcAddress(GetModuleHandle(TEXT("kernel32")), "WriteFile"), myWriteFileHook, nullptr, &hWriteFileHook);
	LhInstallHook(GetProcAddress(GetModuleHandle(TEXT("kernel32")), "SetFilePointer"), mySetFilePointerHook, nullptr, &hSetFilePointerHook);
	LhInstallHook(GetProcAddress(GetModuleHandle(TEXT("kernel32")), "FlushFileBuffers"), myFlushFileBuffersHook, nullptr, &hFlushFileBuffersHook);

	ULONG ACLEntries[1] = { 0 };
	LhSetExclusiveACL(ACLEntries, 1, &hCreateFileWHook);
	LhSetExclusiveACL(ACLEntries, 1, &hCreateFileAHook);
	LhSetExclusiveACL(ACLEntries, 1, &hWriteFileHook);
	LhSetExclusiveACL(ACLEntries, 1, &hSetFilePointerHook);
	LhSetExclusiveACL(ACLEntries, 1, &hFlushFileBuffersHook);

	cout << "[*] File operation hooks installed for ransomware defense." << endl;
}
