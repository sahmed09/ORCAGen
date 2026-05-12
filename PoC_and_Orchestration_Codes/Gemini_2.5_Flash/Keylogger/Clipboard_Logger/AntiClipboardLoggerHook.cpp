#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <easyhook.h>
#include <random> // For random generation if needed for other hooks, kept from original sample
#include <memory> // For std::unique_ptr for memory management (optional, but good practice)

#pragma comment(lib, "EasyHook32.lib") // Or EasyHook64.lib depending on platform

using namespace std;

// Original function pointers
// IMPORTANT: Always use LhHook for original function calls when within a hook.
// This is because EasyHook manages the trampoline to the original function.
// For GetClipboardData, the original function signature:
// HANDLE WINAPI GetClipboardData(UINT uFormat);
// We need a typedef for the original function pointer.
typedef HANDLE(WINAPI* PFN_GetClipboardData)(UINT uFormat);
PFN_GetClipboardData TrueGetClipboardData = nullptr; // Pointer to the original GetClipboardData function

// Decoy clipboard data
const char* DECOY_CLIPBOARD_DATA = "This is decoy clipboard data injected by the cyber deception framework. Original content masked.";

// Hook function for GetClipboardData
HANDLE WINAPI Hook_GetClipboardData(UINT uFormat)
{
	// Output a message to the console of the hooked process (malware POC)
	// This helps in observing the hook's activity.
	cout << "[Hook] GetClipboardData intercepted for format: " << uFormat << endl;

	// We only want to deceive for CF_TEXT format.
	// For other formats, call the original function to maintain normal behavior.
	if (uFormat == CF_TEXT)
	{
		cout << "[Deception] Providing decoy data for CF_TEXT request." << endl;

		// Calculate the size needed for the decoy data, including null terminator
		size_t decoy_len = strlen(DECOY_CLIPBOARD_DATA);
		size_t alloc_size = decoy_len + 1; // +1 for null terminator

		// Allocate global memory for the decoy data.
		// GMEM_MOVEABLE and GMEM_DDESHARE are important for clipboard operations.
		// GMEM_ZEROINIT ensures the memory is null-initialized.
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE | GMEM_ZEROINIT, alloc_size);
		if (hMem == NULL)
		{
			cerr << "[Deception Error] Failed to GlobalAlloc memory for decoy data." << endl;
			// If allocation fails, we might fall back to the original or return NULL.
			// For deception, returning NULL is also a form of deception (empty clipboard).
			// For now, return NULL to indicate failure to provide data.
			return NULL;
		}

		// Lock the global memory to get a pointer to it
		char* pDecoyData = static_cast<char*>(GlobalLock(hMem));
		if (pDecoyData == NULL)
		{
			cerr << "[Deception Error] Failed to GlobalLock memory for decoy data." << endl;
			GlobalFree(hMem); // Free allocated memory if lock fails
			return NULL;
		}

		// Copy the decoy data into the allocated memory
		strcpy_s(pDecoyData, alloc_size, DECOY_CLIPBOARD_DATA);

		// Unlock the global memory. The caller (malware POC) will lock it again.
		GlobalUnlock(hMem);

		// Return the handle to our decoy data.
		// The malware's LogClipboardContent function will then GlobalLock this handle,
		// read our decoy content, and eventually GlobalFree it.
		return hMem;
	}
	else
	{
		// For any other clipboard format, call the original GetClipboardData function.
		// This ensures legitimate applications (if any other part of the hooked process
		// needs it) can still access non-text clipboard data normally.
		return TrueGetClipboardData(uFormat);
	}
}


// Existing hook: CreateFileW (from your sample)
// Kept for context, but not directly related to clipboard deception
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

	// Call the original function
	return CreateFileW(lpFileName,
		dwDesiredAccess,
		dwShareMode,
		lpSecurityAttributes,
		dwCreationDisposition,
		dwFlagsAndAttributes,
		hTemplateFile);
}

// Existing hook: GetAsyncKeyState (from your sample)
// Kept for context, but not directly related to clipboard deception
// Random generator (for GetAsyncKeyState hook)
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dist(0, 99);

SHORT WINAPI myGetAsyncKeyStateHook(int vKey)
{
	SHORT actualState = GetAsyncKeyState(vKey);

	int chance = dist(gen);
	if (chance < 20)
	{
		actualState ^= 0x8000;  // Flip key pressed bit
		cout << "[Deception] Modified GetAsyncKeyState for VK: " << vKey << endl;
	}

	return actualState;
}


extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO* inRemoteInfo)
{
	cout << "[*] Injection started." << endl;

	// Handle user data if any (from your original sample)
	if (inRemoteInfo->UserDataSize > 0)
	{
		// Example of handling user data, though not used for this specific hook
		// DWORD gFreqOffset = *reinterpret_cast<DWORD*>(inRemoteInfo->UserData);
		// cout << "Frequency Offset Received: " << gFreqOffset << endl;
	}

	HOOK_TRACE_INFO hGetClipboardDataHook = { NULL };
	HOOK_TRACE_INFO hCreateFileHook = { NULL }; // For existing CreateFileW hook
	HOOK_TRACE_INFO hAsyncKeyHook = { NULL };   // For existing GetAsyncKeyState hook

	// --- Install GetClipboardData Hook ---
	// Get the address of the original GetClipboardData function from user32.dll
	// Note: Clipboard functions are typically in user32.dll
	TrueGetClipboardData = (PFN_GetClipboardData)GetProcAddress(GetModuleHandle(TEXT("user32")), "GetClipboardData");
	if (TrueGetClipboardData == NULL)
	{
		wstring s(RtlGetLastErrorString());
		wcerr << L"Failed to get address of GetClipboardData: " << s << endl;
		// Optionally, return or exit if crucial API address not found
		return;
	}
	cout << "GetClipboardData function address: " << (void*)TrueGetClipboardData << endl;

	// Install the hook on GetClipboardData
	NTSTATUS result = LhInstallHook(
		TrueGetClipboardData,     // Address of the original function
		Hook_GetClipboardData,    // Address of our hook function
		nullptr,                  // User data for the hook (not used here)
		&hGetClipboardDataHook    // Pointer to HOOK_TRACE_INFO structure
	);

	if (FAILED(result))
	{
		wstring s(RtlGetLastErrorString());
		wcerr << L"Failed to install GetClipboardData hook: " << s << endl;
		return;
	}
	else
	{
		cout << "GetClipboardData hook installed successfully!" << endl;
	}


	// --- Install existing Beep hook (if you want to keep it from original structure) ---
	// PFN_Beep TrueBeep = (PFN_Beep)GetProcAddress(GetModuleHandle(TEXT("kernel32")), "Beep");
	// LhInstallHook(TrueBeep, myBeepHook, nullptr, &hBeepHook); // myBeepHook and PFN_Beep definition needed

	// --- Install existing CreateFileW and GetAsyncKeyState hooks (from your sample) ---
	FARPROC createFileAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "CreateFileW");
	if (createFileAddr) {
		LhInstallHook(createFileAddr, myCreateFileWHook, nullptr, &hCreateFileHook);
		cout << "CreateFileW hook installed." << endl;
	}
	else {
		cerr << "Failed to get CreateFileW address." << endl;
	}

	FARPROC asyncKeyAddr = GetProcAddress(GetModuleHandle(TEXT("user32")), "GetAsyncKeyState");
	if (asyncKeyAddr) {
		LhInstallHook(asyncKeyAddr, myGetAsyncKeyStateHook, nullptr, &hAsyncKeyHook);
		cout << "GetAsyncKeyState hook installed." << endl;
	}
	else {
		cerr << "Failed to get GetAsyncKeyState address." << endl;
	}


	// --- Enable all installed hooks for all threads in the target process ---
	ULONG ACLEntries[3] = { 0 }; // Allocate for 3 hooks: GetClipboardData, CreateFileW, GetAsyncKeyState

	// Set ACL for GetClipboardData hook
	result = LhSetExclusiveACL(ACLEntries, 1, &hGetClipboardDataHook);
	if (FAILED(result))
	{
		wstring s(RtlGetLastErrorString());
		wcerr << L"Failed to set ACL for GetClipboardData hook: " << s << endl;
	}
	else
	{
		cout << "ACL set for GetClipboardData hook." << endl;
	}

	// Set ACL for CreateFileW hook
	result = LhSetExclusiveACL(ACLEntries, 1, &hCreateFileHook);
	if (FAILED(result))
	{
		wstring s(RtlGetLastErrorString());
		wcerr << L"Failed to set ACL for CreateFileW hook: " << s << endl;
	}
	else
	{
		cout << "ACL set for CreateFileW hook." << endl;
	}

	// Set ACL for GetAsyncKeyState hook
	result = LhSetExclusiveACL(ACLEntries, 1, &hAsyncKeyHook);
	if (FAILED(result))
	{
		wstring s(RtlGetLastErrorString());
		wcerr << L"Failed to set ACL for GetAsyncKeyState hook: " << s << endl;
	}
	else
	{
		cout << "ACL set for GetAsyncKeyState hook." << endl;
	}

	cout << "[*] Injection finished. Clipboard data will be deceptive for CF_TEXT." << endl;
}
