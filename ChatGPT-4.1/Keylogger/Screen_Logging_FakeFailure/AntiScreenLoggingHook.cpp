#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <easyhook.h>
#include <iostream>
#include <string>

#pragma comment(lib, "EasyHook32.lib")  // or EasyHook64.lib as needed

using namespace std;

// Pointer for the original BitBlt API
typedef BOOL(WINAPI* BitBltPrototype)(
	HDC hdcDest,
	int nXDest,
	int nYDest,
	int nWidth,
	int nHeight,
	HDC hdcSrc,
	int nXSrc,
	int nYSrc,
	DWORD dwRop
	);

BitBltPrototype TrueBitBlt = nullptr;

// Replace with advanced logic if you wish to allow certain BitBlt requests
bool IsMalwareScreenshot(int nXDest, int nYDest, int nWidth, int nHeight, DWORD dwRop) {
	// Example: block full-screen SRCCOPY
	return (dwRop == SRCCOPY && nXDest == 0 && nYDest == 0 && nWidth >= 300 && nHeight >= 200);
}

// Hooked BitBlt function
BOOL WINAPI myBitBltHook(
	HDC hdcDest,
	int nXDest,
	int nYDest,
	int nWidth,
	int nHeight,
	HDC hdcSrc,
	int nXSrc,
	int nYSrc,
	DWORD dwRop
) {
	// Log every BitBlt attempt
	cout << "[Hooked] BitBlt called. Dest: (" << nXDest << ", " << nYDest
		<< ") Size: (" << nWidth << "x" << nHeight << ") ROP: " << std::hex << dwRop << std::dec << endl;

	// FakeFailure: block suspicious screen logging by returning FALSE
	if (IsMalwareScreenshot(nXDest, nYDest, nWidth, nHeight, dwRop)) {
		cout << "[Active Defense] Blocking BitBlt screen logging attempt (FakeFailure)\n";
		SetLastError(ERROR_ACCESS_DENIED); // Optional: set specific error code
		return FALSE;
	}

	// Otherwise, call the real BitBlt
	return TrueBitBlt(
		hdcDest, nXDest, nYDest, nWidth, nHeight,
		hdcSrc, nXSrc, nYSrc, dwRop
	);
}

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
	cout << "[*] Injection started (Screen Logger Hook)." << endl;

	HOOK_TRACE_INFO hBitBltHook = { NULL };

	// Find the address of BitBlt in gdi32.dll
	HMODULE hGdi32 = GetModuleHandleA("gdi32.dll");
	if (!hGdi32) {
		cerr << "[!] Failed to get gdi32.dll handle." << endl;
		return;
	}
	FARPROC bitbltAddr = GetProcAddress(hGdi32, "BitBlt");
	if (!bitbltAddr) {
		cerr << "[!] Failed to get BitBlt address." << endl;
		return;
	}
	TrueBitBlt = (BitBltPrototype)bitbltAddr;

	// Install the hook
	NTSTATUS result = LhInstallHook(
		bitbltAddr,
		myBitBltHook,
		nullptr,
		&hBitBltHook
	);

	if (FAILED(result)) {
		std::wstring s(RtlGetLastErrorString());
		wcerr << L"Failed to install BitBlt hook: " << s << endl;
		return;
	}
	else {
		cout << "[+] BitBlt hook installed successfully!" << endl;
	}

	// Exclusive ACL: apply to ALL threads in this process (targeted malware)
	ULONG ACLEntries[1] = { 0 };
	LhSetExclusiveACL(ACLEntries, 1, &hBitBltHook);
}
