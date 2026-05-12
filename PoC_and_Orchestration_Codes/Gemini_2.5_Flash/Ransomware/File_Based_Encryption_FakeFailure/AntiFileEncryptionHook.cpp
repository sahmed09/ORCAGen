#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <easyhook.h>
#include <vector>
#include <algorithm> // For std::transform
#include <cctype>    // For std::tolower
#include <Shlwapi.h> // For PathFindExtensionA

#pragma comment(lib, "EasyHook32.lib") // or EasyHook64.lib depending on platform
#pragma comment(lib, "Shlwapi.lib")    // Link with Shlwapi.lib

using namespace std;

// Forward declarations for original API functions
HANDLE(WINAPI* Real_CreateFileA)(
	LPCSTR lpFileName,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes,
	HANDLE hTemplateFile
	) = NULL;

BOOL(WINAPI* Real_WriteFile)(
	HANDLE hFile,
	LPCVOID lpBuffer,
	DWORD nNumberOfBytesToWrite,
	LPDWORD lpNumberOfBytesWritten,
	LPOVERLAPPED lpOverlapped
	) = NULL;

DWORD(WINAPI* Real_SetFilePointer)(
	HANDLE hFile,
	LONG lDistanceToMove,
	PLONG lpDistanceToMoveHigh,
	DWORD dwMoveMethod
	) = NULL;

BOOL(WINAPI* Real_CloseHandle)(
	HANDLE hObject
	) = NULL;

// Configuration for the anti-ransomware deception
// Make these global or pass via REMOTE_ENTRY_INFO if you want the injector to configure them
std::string g_targetEncryptionDirectory = "C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug\\Files";
std::vector<std::string> g_targetEncryptionExtensions = { ".docx", ".xlsx", ".db", ".txt", ".png" };

// Helper function to check if a file path is a target for deception
bool IsTargetForDeception(LPCSTR lpFileName) {
	if (!lpFileName) return false;

	string filePath(lpFileName);

	// Convert path to lowercase for case-insensitive comparison
	std::transform(filePath.begin(), filePath.end(), filePath.begin(),
		[](unsigned char c) { return std::tolower(c); });

	string lowerTargetDir = g_targetEncryptionDirectory;
	std::transform(lowerTargetDir.begin(), lowerTargetDir.end(), lowerTargetDir.begin(),
		[](unsigned char c) { return std::tolower(c); });

	// Check if the file path starts with the target directory (case-insensitive)
	if (filePath.find(lowerTargetDir) == 0) {
		LPCSTR fileExtension = PathFindExtensionA(filePath.c_str());
		if (fileExtension && *fileExtension) {
			std::string ext(fileExtension);
			std::transform(ext.begin(), ext.end(), ext.begin(),
				[](unsigned char c) { return std::tolower(c); });

			for (const auto& targetExt : g_targetEncryptionExtensions) {
				// Ensure targetExt is also lowercase for comparison
				std::string lowerTargetExt = targetExt;
				std::transform(lowerTargetExt.begin(), lowerTargetExt.end(), lowerTargetExt.begin(),
					[](unsigned char c) { return std::tolower(c); });

				if (ext == lowerTargetExt) {
					return true;
				}
			}
		}
	}
	return false;
}

// Hooked CreateFileA
HANDLE WINAPI Hook_CreateFileA(
	LPCSTR lpFileName,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes,
	HANDLE hTemplateFile)
{
	// Check if the file is a target for encryption based on path and extension
	if (IsTargetForDeception(lpFileName)) {
		// If it's a target, we simulate failure
		cerr << "[Deception] Blocking CreateFileA for (potential encryption): " << lpFileName << endl;
		SetLastError(ERROR_ACCESS_DENIED); // Or another appropriate error code
		return INVALID_HANDLE_VALUE;
	}

	// Otherwise, call the original CreateFileA
	return Real_CreateFileA(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
		dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

// Hooked WriteFile
BOOL WINAPI Hook_WriteFile(
	HANDLE hFile,
	LPCVOID lpBuffer,
	DWORD nNumberOfBytesToWrite,
	LPDWORD lpNumberOfBytesWritten,
	LPOVERLAPPED lpOverlapped)
{
	// This hook is more complex as we need to know the file name associated with hFile.
	// EasyHook's context parameter (e.g., LhInstallHook's PVOID InCallback) could be used
	// to store and retrieve file path information linked to a HANDLE.
	// For simplicity in this PoC, we'll use a direct approach assuming that if CreateFileA
	// was targeted, subsequent WriteFile on the same file is also targeted.
	// A more robust solution might involve maintaining a map of HANDLE to filePath.

	// A simpler approach for deception: always return failure if it's writing to a file,
	// and if the process is identified as "malware". This is less precise but effective for PoC.
	// For precise blocking, we'd need to trace the handle.

	// Let's assume that if WriteFile is called on a file that *could* be a target
	// (e.g., if a previous CreateFileA call for a target file resulted in a valid handle
	// due to the hook not firing for some reason, or if we want to block any write
	// to a target file), we block it.
	// This is less precise but serves the "fake failure" for *any* write to a file handle.

	// To make this more robust, you'd need to intercept CreateFileA, store the
	// file path associated with the returned handle, and then check that mapping here.
	// For now, let's just intercept and return fake failure if the caller is the malware
	// and the operation is WriteFile.

	// As per the requirement, we want to return FakeFailure *for ransomware file-based encryption*.
	// Since the malware's `EncryptFile` directly calls `WriteFile`, and we've identified the target
	// directory and extensions, we can intercept here.

	// A more reliable way to identify *which* file is being written to via its HANDLE
	// is often needed for precise blocking. This can be done by hooking NtCreateFile
	// or by using GetFinalPathNameByHandle in Real_WriteFile.
	// For this PoC, we'll assume if it's the target process and it's calling WriteFile,
	// we might want to block. A heuristic approach.

	// Let's block ALL WriteFile calls from the malware process if we suspect it's doing encryption.
	// This is a coarse-grained approach for a PoC.
	// For a real system, you would analyze the handle to determine the file name.

	// To simplify for the PoC, let's use a very simple (and somewhat flawed for real-world)
	// heuristic: if the malware calls WriteFile, and it's not a standard system file, block.
	// A better approach involves GetFinalPathNameByHandleW / GetFinalPathNameByHandleA.

	// Let's make the assumption that if a WriteFile is called and it's part of the
	// malware's encryption routine, we should block it.
	// Since CreateFileA is already hooked, and we want to prevent *encryption*,
	// blocking WriteFile if it seems suspicious is a good step.

	// To provide a 'fake failure' specific to the encryption, we need to know
	// if the handle `hFile` points to one of the target files. This requires more context
	// than a simple `WriteFile` hook provides on its own.

	// For a simple PoC, let's assume that if *any* `WriteFile` occurs after a target
	// `CreateFileA` was allowed (which it wouldn't be if `Hook_CreateFileA` fired),
	// or if we want a second layer of defense, we can block here.

	// To truly make `WriteFile` "fake fail" for the ransomware, we need to ensure the `hFile`
	// corresponds to a file that was previously determined to be a target in `CreateFileA`.
	// Let's try to get the file name from the handle.
	char path[MAX_PATH];
	DWORD dwRet = GetFinalPathNameByHandleA(hFile, path, MAX_PATH, FILE_NAME_NORMALIZED);

	if (dwRet > 0 && dwRet < MAX_PATH && IsTargetForDeception(path)) {
		cout << "[Deception] Blocking WriteFile for (potential encryption): " << path << endl;
		SetLastError(ERROR_DISK_FULL); // Simulate disk full or access denied
		if (lpNumberOfBytesWritten) {
			*lpNumberOfBytesWritten = 0; // Indicate no bytes were written
		}
		return FALSE; // Return failure
	}

	// Call the original WriteFile
	return Real_WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
}

// Hooked SetFilePointer
DWORD WINAPI Hook_SetFilePointer(
	HANDLE hFile,
	LONG lDistanceToMove,
	PLONG lpDistanceToMoveHigh,
	DWORD dwMoveMethod)
{
	char path[MAX_PATH];
	DWORD dwRet = GetFinalPathNameByHandleA(hFile, path, MAX_PATH, FILE_NAME_NORMALIZED);

	if (dwRet > 0 && dwRet < MAX_PATH && IsTargetForDeception(path)) {
		cout << "[Deception] Blocking SetFilePointer for (potential encryption): " << path << endl;
		SetLastError(ERROR_GEN_FAILURE); // Simulate a general failure
		return INVALID_SET_FILE_POINTER; // Return failure
	}

	// Call the original SetFilePointer
	return Real_SetFilePointer(hFile, lDistanceToMove, lpDistanceToMoveHigh, dwMoveMethod);
}

// Hooked CloseHandle
BOOL WINAPI Hook_CloseHandle(HANDLE hObject)
{
	// This hook is critical because if CreateFileA was blocked, the malware
	// might still try to CloseHandle on the INVALID_HANDLE_VALUE it received,
	// or if a legitimate file was opened, we should allow it.
	// Also, if previous operations were allowed for a target file, and CloseHandle is called,
	// we might want to log it or intervene.

	// Check if it's a file handle. Not strictly necessary, but good practice.
	// More robust check: if hObject is INVALID_HANDLE_VALUE, just call original, or return TRUE.
	if (hObject == INVALID_HANDLE_VALUE) {
		// If the malware received an invalid handle from our hooked CreateFileA
		// and tries to close it, let's just "succeed" the close operation for deception.
		// It's already failed conceptually.
		return TRUE; // Deceive by returning success on invalid handle close
	}

	// Check if the handle corresponds to a file that was part of the deception
	char path[MAX_PATH];
	DWORD dwRet = GetFinalPathNameByHandleA(hObject, path, MAX_PATH, FILE_NAME_NORMALIZED);

	if (dwRet > 0 && dwRet < MAX_PATH && IsTargetForDeception(path)) {
		// If we successfully "deceived" the malware by allowing CreateFileA
		// but blocked subsequent WriteFile, then CloseHandle should still work
		// for the malware to keep its illusion.
		// However, if we blocked CreateFileA entirely, CloseHandle shouldn't be called on a valid handle.
		// If the intent is to make the malware *think* it closed the file, we return TRUE.
		cout << "[Deception] Allowing CloseHandle for (target file): " << path << endl;
		return Real_CloseHandle(hObject); // Call original to clean up the actual handle
	}

	// For any other legitimate handle, just call the original.
	return Real_CloseHandle(hObject);
}


extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO* inRemoteInfo)
{
	cout << "[*] Injection started in PID: " << GetCurrentProcessId() << endl;

	// Retrieve original function pointers
	// Note: CreateFileA is in kernel32.dll, WriteFile is in kernel32.dll, SetFilePointer in kernel32.dll, CloseHandle in kernel32.dll
	Real_CreateFileA = (HANDLE(WINAPI*)(LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE))GetProcAddress(GetModuleHandle(TEXT("kernel32")), "CreateFileA");
	Real_WriteFile = (BOOL(WINAPI*)(HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED))GetProcAddress(GetModuleHandle(TEXT("kernel32")), "WriteFile");
	Real_SetFilePointer = (DWORD(WINAPI*)(HANDLE, LONG, PLONG, DWORD))GetProcAddress(GetModuleHandle(TEXT("kernel32")), "SetFilePointer");
	Real_CloseHandle = (BOOL(WINAPI*)(HANDLE))GetProcAddress(GetModuleHandle(TEXT("kernel32")), "CloseHandle");


	// Sanity check
	if (!Real_CreateFileA || !Real_WriteFile || !Real_SetFilePointer || !Real_CloseHandle) {
		wcerr << L"Failed to get original API addresses. Exiting hook." << endl;
		return;
	}

	// Install hooks
	HOOK_TRACE_INFO hCreateFileA = { NULL };
	HOOK_TRACE_INFO hWriteFile = { NULL };
	HOOK_TRACE_INFO hSetFilePointer = { NULL };
	HOOK_TRACE_INFO hCloseHandle = { NULL };

	NTSTATUS result_cf = LhInstallHook(Real_CreateFileA, Hook_CreateFileA, nullptr, &hCreateFileA);
	NTSTATUS result_wf = LhInstallHook(Real_WriteFile, Hook_WriteFile, nullptr, &hWriteFile);
	NTSTATUS result_sfp = LhInstallHook(Real_SetFilePointer, Hook_SetFilePointer, nullptr, &hSetFilePointer);
	NTSTATUS result_ch = LhInstallHook(Real_CloseHandle, Hook_CloseHandle, nullptr, &hCloseHandle);


	if (FAILED(result_cf) || FAILED(result_wf) || FAILED(result_sfp) || FAILED(result_ch))
	{
		wstring s(RtlGetLastErrorString());
		wcerr << L"Failed to install one or more hooks: " << s << endl;
		return;
	}
	else
	{
		cout << "All file operation hooks installed successfully!" << endl;
	}

	// Enable all hooks for all threads in this process
	ULONG ACLEntries[1] = { 0 }; // Apply to all threads
	LhSetExclusiveACL(ACLEntries, 1, &hCreateFileA);
	LhSetExclusiveACL(ACLEntries, 1, &hWriteFile);
	LhSetExclusiveACL(ACLEntries, 1, &hSetFilePointer);
	LhSetExclusiveACL(ACLEntries, 1, &hCloseHandle);

	// Keep the hook DLL loaded
	// If you return from NativeInjectionEntryPoint, the DLL might be unloaded,
	// and hooks would be removed. A common strategy is to block here or
	// have a persistent thread, but for a PoC, keeping a message loop
	// or just letting it run is fine.
	// For this specific scenario where the malware has a loop, the DLL will stay loaded.
}

// Remove the Beep hook and GetAsyncKeyState hook from the previous example
// unless you still want to include that deception.
// The focus here is on file operation deception.