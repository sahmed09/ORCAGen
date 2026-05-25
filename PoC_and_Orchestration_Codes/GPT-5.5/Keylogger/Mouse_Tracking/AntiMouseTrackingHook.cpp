#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <easyhook.h>
#include <random>
#include <string>

#pragma comment(lib, "EasyHook32.lib")  // Use EasyHook64.lib for x64 builds

using namespace std;

// ----------------------------------------------------
// Global deception state
// ----------------------------------------------------
std::random_device rd;
std::mt19937 gen(rd());

std::uniform_int_distribution<int> xDist(100, 900);
std::uniform_int_distribution<int> yDist(100, 700);

HOOK_TRACE_INFO hGetCursorPosHook = { NULL };

// ----------------------------------------------------
// Optional helper: decide whether to deceive this process
// ----------------------------------------------------
bool ShouldDeceiveMouseTracking()
{
	// Since this DLL is injected into the malware PoC process,
	// calls from this process are treated as suspicious.
	//
	// In a larger framework, this function can be extended using:
	// - process name checks
	// - call frequency checks
	// - module allowlists
	// - trusted window/application checks

	return true;
}

// ----------------------------------------------------
// Hooked GetCursorPos
// ----------------------------------------------------
BOOL WINAPI myGetCursorPosHook(LPPOINT lpPoint)
{
	if (lpPoint == nullptr)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	BOOL actualResult = GetCursorPos(lpPoint);

	if (!actualResult)
	{
		return actualResult;
	}

	if (ShouldDeceiveMouseTracking())
	{
		POINT realPoint = *lpPoint;

		lpPoint->x = xDist(gen);
		lpPoint->y = yDist(gen);

		cout << "[Deception] GetCursorPos intercepted." << endl;
		cout << "            Real Position: X=" << realPoint.x
			<< ", Y=" << realPoint.y << endl;
		cout << "            Fake Position: X=" << lpPoint->x
			<< ", Y=" << lpPoint->y << endl;

		return TRUE;
	}

	// Legitimate requests receive the real cursor position.
	return actualResult;
}

// ----------------------------------------------------
// EasyHook entry point
// ----------------------------------------------------
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
	cout << "[*] Anti Mouse Tracking Hook DLL injected." << endl;

	HMODULE user32Module = GetModuleHandleW(L"user32.dll");

	if (user32Module == NULL)
	{
		user32Module = LoadLibraryW(L"user32.dll");
	}

	if (user32Module == NULL)
	{
		cout << "[-] Failed to load user32.dll." << endl;
		return;
	}

	FARPROC getCursorPosAddr = GetProcAddress(user32Module, "GetCursorPos");

	if (getCursorPosAddr == NULL)
	{
		cout << "[-] Failed to resolve GetCursorPos address." << endl;
		return;
	}

	cout << "[*] GetCursorPos address: " << (void*)getCursorPosAddr << endl;

	NTSTATUS result = LhInstallHook(
		getCursorPosAddr,
		myGetCursorPosHook,
		nullptr,
		&hGetCursorPosHook
	);

	if (FAILED(result))
	{
		wstring errorMessage(RtlGetLastErrorString());
		wcerr << L"[-] Failed to install GetCursorPos hook: "
			<< errorMessage << endl;
		return;
	}

	ULONG ACLEntries[1] = { 0 };

	// Exclude the current EasyHook/helper thread from interception.
	// All other threads in the injected process are hooked.
	LhSetExclusiveACL(ACLEntries, 1, &hGetCursorPosHook);

	cout << "[+] GetCursorPos hook installed successfully." << endl;

	while (true)
	{
		Sleep(1000);
	}
}