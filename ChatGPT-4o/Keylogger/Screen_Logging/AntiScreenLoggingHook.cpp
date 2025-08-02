#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <easyhook.h>

#pragma comment(lib, "EasyHook32.lib") // Use EasyHook64.lib for 64-bit

using namespace std;

// Global hook handle
HOOK_TRACE_INFO hBitBltHook = { NULL };

// Pointer to original BitBlt function
typedef BOOL(WINAPI* BitBltFunc)(
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
BitBltFunc originalBitBlt = nullptr;

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
	DWORD dwRop)
{
	// Get current module file name to identify malicious context
	wchar_t moduleName[MAX_PATH] = { 0 };
	GetModuleFileNameW(NULL, moduleName, MAX_PATH);

	// Check if running inside the malware process (example match)
	if (wcsstr(moduleName, L"ClipboardLoggerPOC.exe") != nullptr) {
		cout << "[Hooked BitBlt] Detected suspicious screen capture. Injecting decoy..." << endl;

		// Fill the destination HDC with a gray rectangle instead of real content
		HBRUSH grayBrush = CreateSolidBrush(RGB(192, 192, 192));
		RECT rect = { nXDest, nYDest, nXDest + nWidth, nYDest + nHeight };
		FillRect(hdcDest, &rect, grayBrush);
		DeleteObject(grayBrush);

		return TRUE; // Pretend the capture succeeded
	}

	// Otherwise, call the original BitBlt
	return originalBitBlt(
		hdcDest, nXDest, nYDest, nWidth, nHeight,
		hdcSrc, nXSrc, nYSrc, dwRop
	);
}

// Entry point for EasyHook DLL
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO* inRemoteInfo)
{
	cout << "[*] Injection started in target process." << endl;

	// Hook BitBlt from GDI32
	HMODULE hGDI32 = GetModuleHandleW(L"gdi32.dll");
	if (hGDI32 == nullptr) {
		cerr << "[!] Failed to get gdi32.dll handle." << endl;
		return;
	}

	originalBitBlt = (BitBltFunc)GetProcAddress(hGDI32, "BitBlt");
	if (originalBitBlt == nullptr) {
		cerr << "[!] Failed to resolve BitBlt address." << endl;
		return;
	}

	// Install the hook
	NTSTATUS status = LhInstallHook(
		originalBitBlt,
		myBitBltHook,
		nullptr,
		&hBitBltHook
	);

	if (FAILED(status)) {
		wstring err(RtlGetLastErrorString());
		wcerr << L"[!] Failed to install BitBlt hook: " << err << endl;
		return;
	}

	// Enable the hook for all threads
	ULONG ACLEntries[1] = { 0 };
	LhSetExclusiveACL(ACLEntries, 1, &hBitBltHook);

	cout << "[+] BitBlt hook installed successfully!" << endl;
}
