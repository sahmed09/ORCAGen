#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <easyhook.h>
#include <vector>
#include <algorithm>

#pragma comment(lib, "EasyHook32.lib") // or EasyHook64.lib as appropriate

using namespace std;

// ---- CONFIG ----
const std::wstring TARGET_DIR = L"C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug\\Files";
const std::vector<std::wstring> TARGET_EXTENSIONS = { L".docx", L".xlsx", L".db", L".txt" };

// ---- HELPERS ----
std::wstring ToLower(const std::wstring& str) {
	std::wstring s = str;
	std::transform(s.begin(), s.end(), s.begin(), ::towlower);
	return s;
}

bool EndsWith(const std::wstring& str, const std::wstring& suffix) {
	if (suffix.size() > str.size()) return false;
	return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin());
}

bool IsTargetFile(const std::wstring& path) {
	std::wstring lowpath = ToLower(path);
	// Check if file is in the target directory
	if (lowpath.find(ToLower(TARGET_DIR)) != 0)
		return false;
	// Check for matching extension
	for (auto& ext : TARGET_EXTENSIONS)
		if (EndsWith(lowpath, ext))
			return true;
	return false;
}

// ---- Original API pointers ----
decltype(&CreateFileA) TrueCreateFileA = CreateFileA;
decltype(&WriteFile) TrueWriteFile = WriteFile;
decltype(&SetFilePointer) TrueSetFilePointer = SetFilePointer;
decltype(&FlushFileBuffers) TrueFlushFileBuffers = FlushFileBuffers;

// ---- Tracking handles of protected files ----
std::vector<HANDLE> g_protectedHandles;

// ---- HOOKED APIS ----

// CreateFileA Hook
HANDLE WINAPI MyCreateFileA(
	LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
	std::wstring fileNameW;
	int size = MultiByteToWideChar(CP_ACP, 0, lpFileName, -1, NULL, 0);
	if (size > 0) {
		fileNameW.resize(size - 1);
		MultiByteToWideChar(CP_ACP, 0, lpFileName, -1, &fileNameW[0], size);
	}

	HANDLE hFile = TrueCreateFileA(
		lpFileName, dwDesiredAccess, dwShareMode,
		lpSecurityAttributes, dwCreationDisposition,
		dwFlagsAndAttributes, hTemplateFile);

	if (IsTargetFile(fileNameW)) {
		std::wcout << L"[Ransomware DEFENSE] Protected file opened: " << fileNameW << std::endl;
		if (hFile != INVALID_HANDLE_VALUE)
			g_protectedHandles.push_back(hFile);
	}
	return hFile;
}

// WriteFile Hook
BOOL WINAPI MyWriteFile(
	HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
	LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped)
{
	// Check if this handle is protected
	if (std::find(g_protectedHandles.begin(), g_protectedHandles.end(), hFile) != g_protectedHandles.end()) {
		if (lpNumberOfBytesWritten)* lpNumberOfBytesWritten = nNumberOfBytesToWrite;
		std::cout << "[Ransomware DEFENSE] WriteFile FAKE success (blocked ransomware attempt)." << std::endl;
		return TRUE; // Fake success: block real writes!
	}
	return TrueWriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
}

// SetFilePointer Hook
DWORD WINAPI MySetFilePointer(
	HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod)
{
	if (std::find(g_protectedHandles.begin(), g_protectedHandles.end(), hFile) != g_protectedHandles.end()) {
		std::cout << "[Ransomware DEFENSE] SetFilePointer FAKE success (blocked ransomware attempt)." << std::endl;
		// Return arbitrary valid position (e.g., 0)
		return 0;
	}
	return TrueSetFilePointer(hFile, lDistanceToMove, lpDistanceToMoveHigh, dwMoveMethod);
}

// FlushFileBuffers Hook
BOOL WINAPI MyFlushFileBuffers(HANDLE hFile)
{
	if (std::find(g_protectedHandles.begin(), g_protectedHandles.end(), hFile) != g_protectedHandles.end()) {
		std::cout << "[Ransomware DEFENSE] FlushFileBuffers FAKE success (blocked ransomware attempt)." << std::endl;
		return TRUE; // Fake success
	}
	return TrueFlushFileBuffers(hFile);
}

// Cleanup handles on close (optional)
BOOL WINAPI MyCloseHandle(HANDLE hObject)
{
	auto it = std::find(g_protectedHandles.begin(), g_protectedHandles.end(), hObject);
	if (it != g_protectedHandles.end())
		g_protectedHandles.erase(it);
	return CloseHandle(hObject);
}

// ---- EasyHook entry ----
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
	std::cout << "[*] AntiRansomware DLL Injected. Installing hooks..." << std::endl;

	HOOK_TRACE_INFO hCreateFile = { NULL };
	HOOK_TRACE_INFO hWriteFile = { NULL };
	HOOK_TRACE_INFO hSetFilePointer = { NULL };
	HOOK_TRACE_INFO hFlushBuffers = { NULL };
	HOOK_TRACE_INFO hCloseHandle = { NULL };

	LhInstallHook(GetProcAddress(GetModuleHandleA("kernel32.dll"), "CreateFileA"), MyCreateFileA, NULL, &hCreateFile);
	LhInstallHook(GetProcAddress(GetModuleHandleA("kernel32.dll"), "WriteFile"), MyWriteFile, NULL, &hWriteFile);
	LhInstallHook(GetProcAddress(GetModuleHandleA("kernel32.dll"), "SetFilePointer"), MySetFilePointer, NULL, &hSetFilePointer);
	LhInstallHook(GetProcAddress(GetModuleHandleA("kernel32.dll"), "FlushFileBuffers"), MyFlushFileBuffers, NULL, &hFlushBuffers);
	LhInstallHook(GetProcAddress(GetModuleHandleA("kernel32.dll"), "CloseHandle"), MyCloseHandle, NULL, &hCloseHandle);

	// Enable all hooks for all threads in the process
	ULONG ACLEntries[1] = { 0 };
	LhSetExclusiveACL(ACLEntries, 1, &hCreateFile);
	LhSetExclusiveACL(ACLEntries, 1, &hWriteFile);
	LhSetExclusiveACL(ACLEntries, 1, &hSetFilePointer);
	LhSetExclusiveACL(ACLEntries, 1, &hFlushBuffers);
	LhSetExclusiveACL(ACLEntries, 1, &hCloseHandle);

	std::cout << "[*] AntiRansomware hooks installed. Ransomware encryption attempts will be blocked (FAKE success)." << std::endl;
}
