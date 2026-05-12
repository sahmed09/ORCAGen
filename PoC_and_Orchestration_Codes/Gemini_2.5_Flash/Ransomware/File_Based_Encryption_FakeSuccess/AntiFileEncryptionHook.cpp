#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <easyhook.h>
#include <map>
#include <algorithm> // For std::transform

#pragma comment(lib, "EasyHook32.lib")  // Use EasyHook64.lib for 64-bit

using namespace std;

// Original function pointers
// IMPORTANT: Use the correct calling convention (WINAPI, __stdcall, etc.) for each hooked function.
typedef HANDLE(WINAPI* PFN_CreateFileW)(
	LPCWSTR lpFileName,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes,
	HANDLE hTemplateFile
	);

typedef BOOL(WINAPI* PFN_WriteFile)(
	HANDLE hFile,
	LPCVOID lpBuffer,
	DWORD nNumberOfBytesToWrite,
	LPDWORD lpNumberOfBytesWritten,
	LPOVERLAPPED lpOverlapped
	);

typedef DWORD(WINAPI* PFN_SetFilePointer)(
	HANDLE hFile,
	LONG lDistanceToMove,
	PLONG lpDistanceToMoveHigh,
	DWORD dwMoveMethod
	);

typedef BOOL(WINAPI* PFN_CloseHandle)(
	HANDLE hObject
	);


// Global pointers to original functions
PFN_CreateFileW TrueCreateFileW = NULL;
PFN_WriteFile TrueWriteFile = NULL;
PFN_SetFilePointer TrueSetFilePointer = NULL;
PFN_CloseHandle TrueCloseHandle = NULL;


// Data structures for our deception strategy
// Map to store file paths of handles identified as belonging to target files for encryption.
// We use a map to easily check if a handle is associated with a target file.
// In a real scenario, you might also store file type, original hash, etc.
std::map<HANDLE, std::wstring> DeceptionFileHandles;

// Configuration for target extensions and directories (these should ideally come from configuration)
// These should match what the malware PoC is targeting
std::vector<std::wstring> g_TargetExtensions = {
	L".docx", L".xlsx", L".db", L".txt", L".pdf", L".jpg"
};
// IMPORTANT: Adjust this path to your actual test directory
std::wstring g_TargetDirectory = L"C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug\\Files";


// Helper function to check if a file path is within the target directory
bool IsPathInTargetDirectory(LPCWSTR filePath) {
	if (!filePath) return false;
	std::wstring path = filePath;
	// Convert both to lowercase for case-insensitive comparison
	std::transform(path.begin(), path.end(), path.begin(), ::tolower);
	std::wstring targetDirLower = g_TargetDirectory;
	std::transform(targetDirLower.begin(), targetDirLower.end(), targetDirLower.begin(), ::tolower);

	// Check if the path starts with the target directory path
	return path.rfind(targetDirLower, 0) == 0; // Check if targetDirLower is a prefix of path
}

// Helper function to check if a file has a target extension
bool HasTargetExtension(LPCWSTR filePath) {
	if (!filePath) return false;
	std::wstring fileName = filePath;
	size_t dotPos = fileName.rfind(L'.');
	if (dotPos != std::wstring::npos) {
		std::wstring fileExtension = fileName.substr(dotPos);
		std::transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(), ::tolower);
		for (const auto& ext : g_TargetExtensions) {
			if (fileExtension == ext) {
				return true;
			}
		}
	}
	return false;
}

// Hooked CreateFileW
HANDLE WINAPI DetourCreateFileW(
	LPCWSTR lpFileName,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes,
	HANDLE hTemplateFile)
{
	// Call the original CreateFileW first to get a valid handle
	HANDLE hFile = TrueCreateFileW(lpFileName,
		dwDesiredAccess,
		dwShareMode,
		lpSecurityAttributes,
		dwCreationDisposition,
		dwFlagsAndAttributes,
		hTemplateFile);

	if (hFile != INVALID_HANDLE_VALUE) {
		wcout << L"[Hook:CreateFileW] Intercepted: " << lpFileName;

		// Check if this file operation is potentially part of ransomware's encryption
		// Look for GENERIC_WRITE or GENERIC_ALL access
		if ((dwDesiredAccess & GENERIC_WRITE) || (dwDesiredAccess & FILE_GENERIC_WRITE) || (dwDesiredAccess & GENERIC_ALL)) {
			// Check if it's within our target directory and has a target extension
			if (IsPathInTargetDirectory(lpFileName) && HasTargetExtension(lpFileName)) {
				wcout << L" --> [Target for Deception]";
				// Mark this handle for deception in subsequent WriteFile calls
				DeceptionFileHandles[hFile] = lpFileName;
			}
		}
		wcout << endl;
	}
	else {
		wcout << L"[Hook:CreateFileW] Failed to open: " << lpFileName << L", Error: " << GetLastError() << endl;
	}

	return hFile;
}

// Hooked WriteFile
BOOL WINAPI DetourWriteFile(
	HANDLE hFile,
	LPCVOID lpBuffer,
	DWORD nNumberOfBytesToWrite,
	LPDWORD lpNumberOfBytesWritten,
	LPOVERLAPPED lpOverlapped)
{
	// Check if this handle is marked for deception
	auto it = DeceptionFileHandles.find(hFile);
	if (it != DeceptionFileHandles.end()) {
		wcout << L"[Hook:WriteFile] !!! Deception Triggered for: " << it->second << L" (Fake Success) !!!" << endl;
		// Do NOT call the original WriteFile
		if (lpNumberOfBytesWritten) {
			*lpNumberOfBytesWritten = nNumberOfBytesToWrite; // Make it seem like all bytes were written
		}
		SetLastError(ERROR_SUCCESS); // Ensure no error is reported
		return TRUE; // Return success to the caller (malware)
	}

	// For non-deception files, call the original WriteFile
	BOOL result = TrueWriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
	// cout << "[Hook:WriteFile] Normal operation for handle: " << hFile << ", Result: " << (result ? "TRUE" : "FALSE") << endl;
	return result;
}

// Hooked SetFilePointer (for observation/potential future use)
DWORD WINAPI DetourSetFilePointer(
	HANDLE hFile,
	LONG lDistanceToMove,
	PLONG lpDistanceToMoveHigh,
	DWORD dwMoveMethod)
{
	// Check if this handle is marked for deception
	auto it = DeceptionFileHandles.find(hFile);
	if (it != DeceptionFileHandles.end()) {
		wcout << L"[Hook:SetFilePointer] Intercepted for deception target: " << it->second << endl;
		// You could choose to modify behavior here if needed,
		// but for simple WriteFile deception, just calling original is often fine.
	}

	return TrueSetFilePointer(hFile, lDistanceToMove, lpDistanceToMoveHigh, dwMoveMethod);
}

// Hooked CloseHandle
BOOL WINAPI DetourCloseHandle(HANDLE hObject) {
	// If the handle being closed was marked for deception, remove it from our map
	auto it = DeceptionFileHandles.find(hObject);
	if (it != DeceptionFileHandles.end()) {
		wcout << L"[Hook:CloseHandle] Deception handle closed: " << it->second << endl;
		DeceptionFileHandles.erase(it);
	}
	return TrueCloseHandle(hObject);
}


extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO* inRemoteInfo)
{
	wcout << L"[*] AntiClipboardLoggerHook: Injection started." << endl;

	// Retrieve the process ID of the injected process
	DWORD currentProcessId = GetCurrentProcessId();
	wcout << L"[*] Injected into Process ID: " << currentProcessId << endl;

	// Optional: If you want to send configuration data (like target directory/extensions)
	// from the injector, you can process inRemoteInfo->UserData here.
	// For simplicity, we've hardcoded g_TargetDirectory and g_TargetExtensions for now.
	// In a production system, these would likely be passed via UserData.
	if (inRemoteInfo->UserDataSize > 0) {
		// Example of receiving data (if injector sends it)
		// For example, if it's a wide string for the target directory
		// std::wstring receivedDir((WCHAR*)inRemoteInfo->UserData, inRemoteInfo->UserDataSize / sizeof(WCHAR));
		// g_TargetDirectory = receivedDir;
		// wcout << L"[*] Received Target Directory: " << g_TargetDirectory << endl;
	}


	HOOK_TRACE_INFO hCreateFileHook = { NULL };
	HOOK_TRACE_INFO hWriteFileHook = { NULL };
	HOOK_TRACE_INFO hSetFilePointerHook = { NULL };
	HOOK_TRACE_INFO hCloseHandleHook = { NULL };


	// Get original function addresses from kernel32.dll
	// Use GetModuleHandleW and GetProcAddress with ANSI function names.
	// GetModuleHandleW takes LPCWSTR, GetProcAddress takes LPCSTR for the function name.
	HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
	if (!hKernel32) {
		wcerr << L"Error: Could not get handle to kernel32.dll. Error: " << GetLastError() << endl;
		return;
	}

	TrueCreateFileW = (PFN_CreateFileW)GetProcAddress(hKernel32, "CreateFileW");
	TrueWriteFile = (PFN_WriteFile)GetProcAddress(hKernel32, "WriteFile");
	TrueSetFilePointer = (PFN_SetFilePointer)GetProcAddress(hKernel32, "SetFilePointer");
	TrueCloseHandle = (PFN_CloseHandle)GetProcAddress(hKernel32, "CloseHandle");


	if (TrueCreateFileW && TrueWriteFile && TrueSetFilePointer && TrueCloseHandle) {
		wcout << L"[*] Original API addresses obtained." << endl;

		// Install hooks
		NTSTATUS result;

		result = LhInstallHook(TrueCreateFileW, DetourCreateFileW, NULL, &hCreateFileHook);
		if (FAILED(result)) wcerr << L"Failed to install CreateFileW hook: " << RtlGetLastErrorString() << endl;
		else wcout << L"CreateFileW hook installed successfully." << endl;

		result = LhInstallHook(TrueWriteFile, DetourWriteFile, NULL, &hWriteFileHook);
		if (FAILED(result)) wcerr << L"Failed to install WriteFile hook: " << RtlGetLastErrorString() << endl;
		else wcout << L"WriteFile hook installed successfully." << endl;

		result = LhInstallHook(TrueSetFilePointer, DetourSetFilePointer, NULL, &hSetFilePointerHook);
		if (FAILED(result)) wcerr << L"Failed to install SetFilePointer hook: " << RtlGetLastErrorString() << endl;
		else wcout << L"SetFilePointer hook installed successfully." << endl;

		result = LhInstallHook(TrueCloseHandle, DetourCloseHandle, NULL, &hCloseHandleHook);
		if (FAILED(result)) wcerr << L"Failed to install CloseHandle hook: " << RtlGetLastErrorString() << endl;
		else wcout << L"CloseHandle hook installed successfully." << endl;

		// Enable all hooks for all threads in the target process
		ULONG ACLEntries[1] = { 0 }; // 0 means all threads
		LhSetExclusiveACL(ACLEntries, 1, &hCreateFileHook);
		LhSetExclusiveACL(ACLEntries, 1, &hWriteFileHook);
		LhSetExclusiveACL(ACLEntries, 1, &hSetFilePointerHook);
		LhSetExclusiveACL(ACLEntries, 1, &hCloseHandleHook);

		wcout << L"[*] All file API hooks enabled." << endl;

	}
	else {
		wcerr << L"Error: Could not find all necessary API addresses." << endl;
	}

	// Keep the DLL loaded in the target process
	// This loop prevents the DLL from being unloaded immediately,
	// which would remove the hooks. In a real scenario, you might
	// have a more robust way to manage the DLL's lifecycle.
	while (true) {
		Sleep(1000);
	}
}