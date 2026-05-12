#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <easyhook.h>
#include <random>

#pragma comment(lib, "EasyHook32.lib") // Or EasyHook64.lib depending on platform

using namespace std;

// Random generator (from your sample for GetAsyncKeyState, kept for completeness)
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dist(0, 99);

// Global variable to store the original GetCursorPos function pointer
// This is crucial for calling the original API from our hook
BOOL(WINAPI* g_pOriginalGetCursorPos)(LPPOINT lpPoint) = NULL;

// --- Hook Function for GetCursorPos ---
BOOL WINAPI myGetCursorPosHook(LPPOINT lpPoint)
{
	// Output a message indicating the hook was triggered
	cout << "[+] GetCursorPos Hook Triggered!" << endl;

	// Active Defense Strategy: Mislead the malware by returning deceptive coordinates
	// We'll simulate a fixed or "blocked" position for the malware.
	// For a simple deception, we can always return (0,0) and true.
	if (lpPoint != nullptr)
	{
		lpPoint->x = 0; // Deceptive X coordinate
		lpPoint->y = 0; // Deceptive Y coordinate
		cout << "[Deception] Reported Mouse Position: X=0, Y=0 (Blocked for malware)" << endl;
	}

	// Always return TRUE to indicate success to the caller (malware in this case)
	return TRUE;

	// If you wanted to only intercept for the malware and allow others to pass through,
	// you'd need a more sophisticated check (e.g., based on process ID or module name).
	// For this specific scenario where the malware is the primary target,
	// and we want to actively mislead its mouse tracking, always returning (0,0) is effective.
	// If you *really* needed to pass through legitimate calls, you'd integrate the following:
	// return g_pOriginalGetCursorPos(lpPoint);
}


// --- Existing Hook Function from your sample (Beep) ---
DWORD gFreqOffset = 0; // Kept from your sample
BOOL WINAPI myBeepHook(DWORD dwFreq, DWORD dwDuration)
{
	cout << "[+] BeepHook triggered!" << endl;
	cout << "Original Frequency: " << dwFreq << ", Duration: " << dwDuration << endl;
	return Beep(dwFreq + gFreqOffset, dwDuration);
}


// --- Existing Hook Function from your sample (CreateFileW) ---
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

	return CreateFileW(lpFileName,
		dwDesiredAccess,
		dwShareMode,
		lpSecurityAttributes,
		dwCreationDisposition,
		dwFlagsAndAttributes,
		hTemplateFile);
}

// --- Existing Hook Function from your sample (GetAsyncKeyState) ---
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

	// --- Initialize Hooks ---
	HOOK_TRACE_INFO hBeepHook = { NULL }; // For the Beep hook (from your initial project structure)
	HOOK_TRACE_INFO hGetCursorPosHook = { NULL }; // For the new GetCursorPos hook
	HOOK_TRACE_INFO hCreateFileHook = { NULL }; // From your sample
	HOOK_TRACE_INFO hAsyncKeyHook = { NULL };   // From your sample

	// Handle UserData for Beep offset (from your initial project structure)
	if (inRemoteInfo->UserDataSize == sizeof(DWORD))
	{
		gFreqOffset = *reinterpret_cast<DWORD*>(inRemoteInfo->UserData);
		cout << "Frequency Offset Received: " << gFreqOffset << endl;
	}

	// --- Install Beep Hook (from your initial project structure) ---
	FARPROC beepAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "Beep");
	cout << "Beep function address: " << (void*)beepAddr << endl;
	NTSTATUS resultBeep = LhInstallHook(beepAddr, myBeepHook, nullptr, &hBeepHook);
	if (FAILED(resultBeep))
	{
		wstring s(RtlGetLastErrorString());
		wcerr << L"Failed to install Beep hook: " << s << endl;
	}
	else
	{
		cout << "Beep hook installed successfully!" << endl;
	}

	// --- Install GetCursorPos Hook ---
	// 1. Get the address of the original GetCursorPos function.
	FARPROC getCursorPosAddr = GetProcAddress(GetModuleHandle(TEXT("user32")), "GetCursorPos");
	if (getCursorPosAddr == nullptr) {
		wcerr << L"Failed to get address for GetCursorPos." << endl;
		return;
	}
	cout << "GetCursorPos function address: " << (void*)getCursorPosAddr << endl;

	// 2. Cast the original function pointer to our global variable.
	g_pOriginalGetCursorPos = (BOOL(WINAPI*)(LPPOINT))getCursorPosAddr;

	// 3. Install the hook.
	NTSTATUS resultGetCursorPos = LhInstallHook(getCursorPosAddr, myGetCursorPosHook, nullptr, &hGetCursorPosHook);

	if (FAILED(resultGetCursorPos))
	{
		wstring s(RtlGetLastErrorString());
		wcerr << L"Failed to install GetCursorPos hook: " << s << endl;
	}
	else
	{
		cout << "GetCursorPos hook installed successfully!" << endl;
	}


	// --- Install CreateFileW Hook (from your sample) ---
	FARPROC createFileAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "CreateFileW");
	NTSTATUS resultCreateFile = LhInstallHook(createFileAddr, myCreateFileWHook, nullptr, &hCreateFileHook);
	if (FAILED(resultCreateFile))
	{
		wstring s(RtlGetLastErrorString());
		wcerr << L"Failed to install CreateFileW hook: " << s << endl;
	}
	else {
		cout << "CreateFileW hook installed successfully!" << endl;
	}

	// --- Install GetAsyncKeyState Hook (from your sample) ---
	FARPROC asyncKeyAddr = GetProcAddress(GetModuleHandle(TEXT("user32")), "GetAsyncKeyState");
	NTSTATUS resultAsyncKey = LhInstallHook(asyncKeyAddr, myGetAsyncKeyStateHook, nullptr, &hAsyncKeyHook);
	if (FAILED(resultAsyncKey))
	{
		wstring s(RtlGetLastErrorString());
		wcerr << L"Failed to install GetAsyncKeyState hook: " << s << endl;
	}
	else {
		cout << "GetAsyncKeyState hook installed successfully!" << endl;
	}


	// --- Enable Hooks ---
	ULONG ACLEntries[1] = { 0 }; // Apply to all threads in the process

	// Enable the Beep hook
	LhSetExclusiveACL(ACLEntries, 1, &hBeepHook);
	// Enable the GetCursorPos hook
	LhSetExclusiveACL(ACLEntries, 1, &hGetCursorPosHook);
	// Enable the CreateFileW hook
	LhSetExclusiveACL(ACLEntries, 1, &hCreateFileHook);
	// Enable the GetAsyncKeyState hook
	LhSetExclusiveACL(ACLEntries, 1, &hAsyncKeyHook);

	cout << "[*] All specified hooks enabled." << endl;
}