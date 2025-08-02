#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <easyhook.h>
#include <iostream>
#include <string>

#pragma comment(lib, "EasyHook32.lib")  // or EasyHook64.lib for x64

using namespace std;

// Function pointer type for GetClipboardData
typedef HANDLE(WINAPI* GetClipboardData_t)(UINT uFormat);

// Global to store the real GetClipboardData
GetClipboardData_t TrueGetClipboardData = nullptr;

// Decoy content
const wchar_t* DECOY_TEXT = L"FAKE_PASSWORD_123!";

// Hooked GetClipboardData function
HANDLE WINAPI MyGetClipboardDataHook(UINT uFormat)
{
	// We only want to deceive for Unicode text
	if (uFormat == CF_UNICODETEXT) {
		wcout << L"[Deception] Returning decoy clipboard data!" << endl;

		size_t sizeInBytes = (wcslen(DECOY_TEXT) + 1) * sizeof(wchar_t);
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, sizeInBytes);
		if (hMem) {
			LPVOID p = GlobalLock(hMem);
			if (p) {
				memcpy(p, DECOY_TEXT, sizeInBytes);
				GlobalUnlock(hMem);
			}
			// Return our decoy clipboard handle
			return hMem;
		}
		return nullptr;
	}

	// Otherwise, call the original function for all other formats
	return TrueGetClipboardData(uFormat);
}

// === EasyHook DLL entry point ===
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO* inRemoteInfo)
{
	HOOK_TRACE_INFO hHook = { NULL };

	// Get address of GetClipboardData
	HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
	if (!hUser32) {
		wcout << L"[!] user32.dll not loaded!" << endl;
		return;
	}
	FARPROC pGetClipboardData = GetProcAddress(hUser32, "GetClipboardData");
	if (!pGetClipboardData) {
		wcout << L"[!] GetClipboardData not found!" << endl;
		return;
	}
	TrueGetClipboardData = (GetClipboardData_t)pGetClipboardData;

	// Install hook
	NTSTATUS result = LhInstallHook(
		pGetClipboardData,
		MyGetClipboardDataHook,
		nullptr,
		&hHook
	);

	if (FAILED(result)) {
		wcout << L"[!] Hook install failed: " << RtlGetLastErrorString() << endl;
		return;
	}

	ULONG ACLEntries[1] = { 0 };
	// Apply exclusive ACL to hook everything in the process
	LhSetExclusiveACL(ACLEntries, 1, &hHook);

	wcout << L"[+] GetClipboardData hook installed successfully!" << endl;
}
