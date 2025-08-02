#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <easyhook.h>
#include <iostream>
#include <string>
#include <random>

#pragma comment(lib, "EasyHook32.lib") // or EasyHook64.lib as appropriate

using namespace std;

// Pointer type for original BitBlt
typedef BOOL(WINAPI* BitBlt_t)(
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

BitBlt_t True_BitBlt = BitBlt; // fallback (will be overwritten by LhGetHookBypassAddress)

HOOK_TRACE_INFO hBitBltHook = { NULL };

// Helper: Fill the HDC region with a decoy (solid color or random noise)
void DrawDecoy(HDC hdc, int width, int height)
{
	// Simple: fill with solid red, or generate noise
	HBRUSH hBrush = CreateSolidBrush(RGB(255, 0, 0)); // Red
	RECT r = { 0, 0, width, height };
	FillRect(hdc, &r, hBrush);
	DeleteObject(hBrush);
	// (Optional) You can implement a more advanced decoy here (noise, watermark, fake UI, etc.)
}

// Our hooked BitBlt function
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
	// Detect virtual screen capture pattern
	int vx = GetSystemMetrics(SM_XVIRTUALSCREEN);
	int vy = GetSystemMetrics(SM_YVIRTUALSCREEN);
	int vw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	int vh = GetSystemMetrics(SM_CYVIRTUALSCREEN);

	// If full desktop capture, mislead
	if (nXDest == 0 && nYDest == 0 &&
		nXSrc == vx && nYSrc == vy &&
		nWidth == vw && nHeight == vh &&
		(dwRop & SRCCOPY)) // you can further restrict by checking dwRop
	{
		// Optionally: log the event
		cout << "[Deception] BitBlt screen capture intercepted! Returning decoy image." << endl;
		DrawDecoy(hdcDest, nWidth, nHeight);
		return TRUE; // Pretend capture was successful
	}
	// Otherwise, call the real BitBlt for legitimate usage
	return True_BitBlt(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, dwRop);
}

// DLL entry point for EasyHook
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO* inRemoteInfo)
{
	cout << "[*] AntiScreenLoggerHook injected." << endl;

	// Install BitBlt hook
	// Corrected: use ANSI string literal
	HMODULE hGdi32 = GetModuleHandleA("gdi32.dll");
	FARPROC bitbltAddr = GetProcAddress(hGdi32, "BitBlt");
	if (!bitbltAddr) {
		cerr << "[-] Failed to get BitBlt address!" << endl;
		return;
	}

	// Save trampoline for original BitBlt
	True_BitBlt = (BitBlt_t)bitbltAddr;

	NTSTATUS res = LhInstallHook(
		bitbltAddr,
		myBitBltHook,
		nullptr,
		&hBitBltHook);

	if (FAILED(res)) {
		wstring s(RtlGetLastErrorString());
		wcerr << L"[-] Failed to install BitBlt hook: " << s << endl;
		return;
	}

	cout << "[+] BitBlt hook installed!" << endl;

	// Enable the hook for all threads (or restrict if desired)
	ULONG ACLEntries[1] = { 0 };
	LhSetExclusiveACL(ACLEntries, 1, &hBitBltHook);
}
