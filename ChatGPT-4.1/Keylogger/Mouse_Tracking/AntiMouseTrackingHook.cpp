#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <easyhook.h>
#include <thread>

#pragma comment(lib, "EasyHook32.lib") // or EasyHook64.lib

using namespace std;

// Pointer to the original GetCursorPos
typedef BOOL(WINAPI* GetCursorPos_t)(LPPOINT lpPoint);
GetCursorPos_t TrueGetCursorPos = nullptr;

// Custom hook for GetCursorPos
BOOL WINAPI myGetCursorPosHook(LPPOINT lpPoint)
{
	// Get the current thread ID
	DWORD currThreadId = GetCurrentThreadId();
	DWORD mainThreadId = GetCurrentProcessId(); // For demonstration

	// Here you could add more robust logic for detecting "malicious" requests
	// Example: block if called from a specific thread, or after repeated accesses, etc.
	// For PoC, let's block ALL calls and print a block message

	std::cout << "[Active Defense] Blocked mouse tracking attempt via GetCursorPos!" << std::endl;

	// Optionally: Provide fake/decoy mouse data to mislead the malware
	if (lpPoint)
	{
		lpPoint->x = 0; // Decoy/fixed position
		lpPoint->y = 0;
	}
	SetLastError(ERROR_ACCESS_DENIED);
	return FALSE; // Indicate failure
	// To allow: return TrueGetCursorPos(lpPoint);
}

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO* inRemoteInfo)
{
	std::cout << "[*] Injection started (AntiMouseLoggerHook)." << std::endl;

	HOOK_TRACE_INFO hGetCursorPosHook = { NULL };

	// Get the address of GetCursorPos from user32.dll
	HMODULE hUser32 = GetModuleHandleA("user32");
	if (!hUser32)
	{
		std::cerr << "[!] Failed to get handle for user32.dll" << std::endl;
		return;
	}
	FARPROC getCursorPosAddr = GetProcAddress(hUser32, "GetCursorPos");
	if (!getCursorPosAddr)
	{
		std::cerr << "[!] Failed to get address of GetCursorPos" << std::endl;
		return;
	}
	TrueGetCursorPos = (GetCursorPos_t)getCursorPosAddr;

	// Install the hook
	NTSTATUS result = LhInstallHook(
		getCursorPosAddr,
		myGetCursorPosHook,
		nullptr,
		&hGetCursorPosHook
	);

	if (FAILED(result))
	{
		std::wstring err(RtlGetLastErrorString());
		std::wcerr << L"[!] Failed to install GetCursorPos hook: " << err << std::endl;
		return;
	}
	else
	{
		std::cout << "[+] GetCursorPos hook installed successfully!" << std::endl;
	}

	// Enable for all threads in the process (exclusive ACL, only for this process)
	ULONG ACLEntries[1] = { 0 };
	LhSetExclusiveACL(ACLEntries, 1, &hGetCursorPosHook);

	// DLL main thread goes idle
	while (true)
		Sleep(1000);
}
