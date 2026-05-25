#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <easyhook.h>
#include <random>

#pragma comment(lib, "EasyHook32.lib")  // or EasyHook64.lib depending on architecture

using namespace std;

// Standard random generation engines for generating deceptive coordinates
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> x_dist(100, 500);   // Simulated bounded X coordinates
std::uniform_int_distribution<> y_dist(100, 500);   // Simulated bounded Y coordinates

// ============================================================================
// Hook Function for GetCursorPos
// ============================================================================
BOOL WINAPI myGetCursorPosHook(LPPOINT lpPoint)
{
	// 1. Output the alert/block message to standard out to indicate active defense response
	cout << "\n[Active Defense] GetCursorPos tracking intercepted and mitigated!" << endl;

	if (lpPoint != nullptr)
	{
		// 2. Supply fake coordinates to feed deceptive data to the tracker
		lpPoint->x = x_dist(gen);
		lpPoint->y = y_dist(gen);

		cout << "[Deception] Spoofed tracking telemetry returned -> X: " << lpPoint->x
			<< ", Y: " << lpPoint->y << endl;

		// Return TRUE to trick the malware into believing the API completed successfully
		return TRUE;
	}

	// Fallback to avoid safety violations if a null pointer is passed
	return FALSE;
}

// ============================================================================
// Existing Reference Hooks
// ============================================================================
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

SHORT WINAPI myGetAsyncKeyStateHook(int vKey)
{
	SHORT actualState = GetAsyncKeyState(vKey);
	std::uniform_int_distribution<> dist(0, 99);

	int chance = dist(gen);
	if (chance < 20)
	{
		actualState ^= 0x8000;  // Flip key pressed bit
		cout << "[Deception] Modified GetAsyncKeyState for VK: " << vKey << endl;
	}

	return actualState;
}

// ============================================================================
// Injection Entry Point
// ============================================================================
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
	cout << "[*] Injection started." << endl;

	HOOK_TRACE_INFO hCreateFileHook = { NULL };
	HOOK_TRACE_INFO hAsyncKeyHook = { NULL };
	HOOK_TRACE_INFO hCursorPosHook = { NULL }; // New hook trace info

	// 1. Resolve and install hook for GetCursorPos (located in user32.dll)
	FARPROC cursorPosAddr = GetProcAddress(GetModuleHandle(TEXT("user32")), "GetCursorPos");
	NTSTATUS cursorResult = LhInstallHook(cursorPosAddr, myGetCursorPosHook, nullptr, &hCursorPosHook);

	if (FAILED(cursorResult))
	{
		wstring s(RtlGetLastErrorString());
		wcerr << L"[-] Failed to install GetCursorPos hook: " << s << endl;
	}
	else
	{
		cout << "[+] GetCursorPos hook installed successfully!" << endl;
	}

	// 2. Install remaining hooks
	FARPROC createFileAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "CreateFileW");
	LhInstallHook(createFileAddr, myCreateFileWHook, nullptr, &hCreateFileHook);

	FARPROC asyncKeyAddr = GetProcAddress(GetModuleHandle(TEXT("user32")), "GetAsyncKeyState");
	LhInstallHook(asyncKeyAddr, myGetAsyncKeyStateHook, nullptr, &hAsyncKeyHook);

	// 3. Configure Exclusive ACLs to activate hooks across the target process context
	ULONG ACLEntries[1] = { 0 };
	LhSetExclusiveACL(ACLEntries, 1, &hCursorPosHook);
	LhSetExclusiveACL(ACLEntries, 1, &hCreateFileHook);
	LhSetExclusiveACL(ACLEntries, 1, &hAsyncKeyHook);
}