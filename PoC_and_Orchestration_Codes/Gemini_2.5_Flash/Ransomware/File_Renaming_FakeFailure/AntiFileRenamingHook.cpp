#include "pch.h"
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm> // For std::transform
#include <easyhook.h>
#include <random>
#include <Shlwapi.h> // For PathFindExtensionW (Link with Shlwapi.lib)

#pragma comment(lib, "EasyHook32.lib") // Or EasyHook64.lib depending on platform
#pragma comment(lib, "Shlwapi.lib")    // Required for PathFindExtensionW

using namespace std;

// --- Global Variables and Configuration ---

// Global frequency offset for Beep hook (from previous example)
DWORD gFreqOffset = 0;

// List of suspicious file extensions that indicate ransomware activity
const vector<wstring> SUSPICIOUS_EXTENSIONS = {
	L".locked",
	L".enc",
	L".encrypted",
	L".rvl", // Example for .REvil, often truncated or different
	L".crypt",
	L".ransom",
	L".xyz", // Add more as needed
	L".crypted",
	L".evil",
	L".boom"
};

// Random generator for deceptive GetAsyncKeyState (from previous example)
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dist(0, 99);

// --- Original Function Pointers (Trampolines for EasyHook) ---

// Define function pointers for the original APIs
typedef BOOL(WINAPI* PFN_MoveFileExW)(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, DWORD dwFlags);
typedef BOOL(WINAPI* PFN_MoveFileW)(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName);

// Global pointers to the original functions. EasyHook will populate these.
PFN_MoveFileExW Real_MoveFileExW = nullptr;
PFN_MoveFileW Real_MoveFileW = nullptr;

// --- Helper Functions ---

// Converts a wide string to lowercase for case-insensitive comparison
wstring ToLower(const wstring& str) {
	wstring lower_str = str;
	transform(lower_str.begin(), lower_str.end(), lower_str.begin(), ::tolower);
	return lower_str;
}

// Checks if a given file extension is in our list of suspicious extensions
bool IsSuspiciousExtension(LPCWSTR filePath) {
	if (filePath == nullptr) {
		return false;
	}

	// PathFindExtensionW returns a pointer to the '.' character of the extension
	LPCWSTR extension = PathFindExtensionW(filePath);

	if (extension == nullptr || *extension == L'\0') {
		// No extension found
		return false;
	}

	// Convert the found extension to lowercase for case-insensitive comparison
	wstring ext_str = ToLower(extension);

	// Check if the lowercase extension is in our suspicious list
	for (const wstring& suspicious_ext : SUSPICIOUS_EXTENSIONS) {
		if (ext_str == suspicious_ext) {
			return true;
		}
	}
	return false;
}

// --- Hook Functions ---

// Hook for Beep (from previous example)
BOOL WINAPI myBeepHook(DWORD dwFreq, DWORD dwDuration)
{
	wcout << L"[+] BeepHook triggered!" << endl;
	wcout << L"Original Frequency: " << dwFreq << L", Duration: " << dwDuration << endl;
	// Call the original Beep function with an offset (if any)
	return Beep(dwFreq + gFreqOffset, dwDuration);
}

// Deceptive GetAsyncKeyState with selective printing (from previous example)
SHORT WINAPI myGetAsyncKeyStateHook(int vKey)
{
	SHORT actualState = GetAsyncKeyState(vKey);

	int chance = dist(gen);
	if (chance < 20) // 20% chance to modify the state
	{
		actualState ^= 0x8000;  // Flip key pressed bit (most significant bit)
		wcout << L"[Deception] Modified GetAsyncKeyState for VK: " << vKey << endl;
	}

	return actualState;
}

// Hook for MoveFileExW
BOOL WINAPI myMoveFileExWHook(
	LPCWSTR lpExistingFileName,
	LPCWSTR lpNewFileName,
	DWORD dwFlags)
{
	wcout << L"[Hook] MoveFileExW intercepted." << endl;
	wcout << L"       Old Path: \"" << (lpExistingFileName ? lpExistingFileName : L"NULL") << L"\"\n";
	wcout << L"       New Path: \"" << (lpNewFileName ? lpNewFileName : L"NULL") << L"\"\n";

	if (lpNewFileName != nullptr && IsSuspiciousExtension(lpNewFileName))
	{
		wcout << L"       [!!! BLOCKED !!!] Suspicious file rename detected! Preventing operation.\n";
		// Simulate failure by returning FALSE and setting a relevant error code
		SetLastError(ERROR_ACCESS_DENIED); // Or ERROR_SHARING_VIOLATION, ERROR_FILE_NOT_FOUND etc.
		return FALSE; // Block the rename operation
	}
	else
	{
		wcout << L"       [ALLOW] Legitimate file rename detected. Allowing operation.\n";
		// Call the original MoveFileExW function
		return Real_MoveFileExW(lpExistingFileName, lpNewFileName, dwFlags);
	}
}

// Hook for MoveFileW
BOOL WINAPI myMoveFileWHook(
	LPCWSTR lpExistingFileName,
	LPCWSTR lpNewFileName)
{
	wcout << L"[Hook] MoveFileW intercepted." << endl;
	wcout << L"       Old Path: \"" << (lpExistingFileName ? lpExistingFileName : L"NULL") << L"\"\n";
	wcout << L"       New Path: \"" << (lpNewFileName ? lpNewFileName : L"NULL") << L"\"\n";

	if (lpNewFileName != nullptr && IsSuspiciousExtension(lpNewFileName))
	{
		wcout << L"       [!!! BLOCKED !!!] Suspicious file rename detected! Preventing operation.\n";
		SetLastError(ERROR_ACCESS_DENIED);
		return FALSE; // Block the rename operation
	}
	else
	{
		wcout << L"       [ALLOW] Legitimate file rename detected. Allowing operation.\n";
		// Call the original MoveFileW function
		return Real_MoveFileW(lpExistingFileName, lpNewFileName);
	}
}

// --- Entry Point for EasyHook Injection ---

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO* inRemoteInfo)
{
	wcout << L"[*] Injection started in target process." << endl;

	// Process UserData if provided (from InjectAntiClipboardHook.cpp)
	if (inRemoteInfo->UserDataSize == sizeof(DWORD))
	{
		gFreqOffset = *reinterpret_cast<DWORD*>(inRemoteInfo->UserData);
		wcout << L"Frequency Offset Received: " << gFreqOffset << endl;
	}

	// Initialize HOOK_TRACE_INFO structures for each hook
	HOOK_TRACE_INFO hBeepHook = { NULL };
	HOOK_TRACE_INFO hAsyncKeyHook = { NULL };
	HOOK_TRACE_INFO hMoveFileExWHook = { NULL };
	HOOK_TRACE_INFO hMoveFileWHook = { NULL };

	NTSTATUS result;

	// 1. Install Beep hook
	FARPROC beepAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "Beep");
	if (beepAddr) {
		result = LhInstallHook(beepAddr, myBeepHook, nullptr, &hBeepHook);
		if (FAILED(result)) {
			wcout << L"[-] Failed to install Beep hook: " << RtlGetLastErrorString() << endl;
		}
		else {
			wcout << L"[+] Beep hook installed successfully!" << endl;
		}
	}
	else {
		wcout << L"[-] Could not find Beep function address." << endl;
	}

	// 2. Install GetAsyncKeyState hook
	FARPROC asyncKeyAddr = GetProcAddress(GetModuleHandle(TEXT("user32")), "GetAsyncKeyState");
	if (asyncKeyAddr) {
		result = LhInstallHook(asyncKeyAddr, myGetAsyncKeyStateHook, nullptr, &hAsyncKeyHook);
		if (FAILED(result)) {
			wcout << L"[-] Failed to install GetAsyncKeyState hook: " << RtlGetLastErrorString() << endl;
		}
		else {
			wcout << L"[+] GetAsyncKeyState hook installed successfully!" << endl;
		}
	}
	else {
		wcout << L"[-] Could not find GetAsyncKeyState function address." << endl;
	}

	// 3. Install MoveFileExW hook
	FARPROC moveFileExWAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "MoveFileExW");
	if (moveFileExWAddr) {
		result = LhInstallHook(moveFileExWAddr, myMoveFileExWHook, &Real_MoveFileExW, &hMoveFileExWHook);
		if (FAILED(result)) {
			wcout << L"[-] Failed to install MoveFileExW hook: " << RtlGetLastErrorString() << endl;
		}
		else {
			wcout << L"[+] MoveFileExW hook installed successfully!" << endl;
		}
	}
	else {
		wcout << L"[-] Could not find MoveFileExW function address." << endl;
	}

	// 4. Install MoveFileW hook
	FARPROC moveFileWAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "MoveFileW");
	if (moveFileWAddr) {
		result = LhInstallHook(moveFileWAddr, myMoveFileWHook, &Real_MoveFileW, &hMoveFileWHook);
		if (FAILED(result)) {
			wcout << L"[-] Failed to install MoveFileW hook: " << RtlGetLastErrorString() << endl;
		}
		else {
			wcout << L"[+] MoveFileW hook installed successfully!" << endl;
		}
	}
	else {
		wcout << L"[-] Could not find MoveFileW function address." << endl;
	}

	// Apply the hooks to all threads in the target process
	// Note: All hooks should be activated together for consistency.
	ULONG ACLEntries[1] = { 0 }; // 0 means all threads
	LhSetExclusiveACL(ACLEntries, 1, &hBeepHook);
	LhSetExclusiveACL(ACLEntries, 1, &hAsyncKeyHook);
	LhSetExclusiveACL(ACLEntries, 1, &hMoveFileExWHook);
	LhSetExclusiveACL(ACLEntries, 1, &hMoveFileWHook);

	wcout << L"[*] All hooks installed and activated. Waiting for target process to exit..." << endl;

	// Keep the DLL loaded and hooks active until the target process exits.
	// In a real scenario, you might have a communication channel here.
	// For this PoC, we just wait indefinitely.
	// You can also use RhWaitForClientExit(0) in the injector to wait.
	// This loop is primarily for debugging output, ensuring the DLL stays loaded.
	while (true) {
		Sleep(1000);
	}
}
