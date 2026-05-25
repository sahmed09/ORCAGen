#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <easyhook.h>

#pragma comment(lib, "EasyHook32.lib") // Use EasyHook64.lib for 64-bit builds

using namespace std;

// Global Hook Info
HOOK_TRACE_INFO hCursorHook = { NULL };

// Hooked GetCursorPos function
BOOL WINAPI myGetCursorPosHook(LPPOINT lpPoint)
{
	// Deception logic: pretend call failed, or spoof data
	cout << "[BLOCKED] Mouse tracking attempt detected. Returning fake position." << endl;

	if (lpPoint != nullptr) {
		lpPoint->x = 9999;
		lpPoint->y = 9999;
	}

	return TRUE; // Still return TRUE to avoid crashing poorly-written malware
}

// Entry point called upon DLL injection
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO* inRemoteInfo)
{
	cout << "[*] Injection started into process.\n";

	// Install hook on GetCursorPos
	FARPROC target = GetProcAddress(GetModuleHandle(TEXT("user32")), "GetCursorPos");
	if (target == nullptr) {
		cerr << "[-] Failed to get GetCursorPos address." << endl;
		return;
	}

	NTSTATUS result = LhInstallHook(target, myGetCursorPosHook, nullptr, &hCursorHook);
	if (FAILED(result)) {
		wcerr << L"[-] Failed to install hook on GetCursorPos: " << RtlGetLastErrorString() << endl;
		return;
	}

	// Apply to all threads in process
	ULONG ACLEntries[1] = { 0 };
	LhSetExclusiveACL(ACLEntries, 1, &hCursorHook);

	cout << "[+] GetCursorPos hook installed successfully." << endl;
}
