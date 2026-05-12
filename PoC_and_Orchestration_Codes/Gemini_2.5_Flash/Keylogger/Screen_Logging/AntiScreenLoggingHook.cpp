#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <easyhook.h>
#include <random>
#include <vector>

#pragma comment(lib, "EasyHook32.lib")

using namespace std;

// Function pointers for original APIs
typedef BOOL(WINAPI* PBitBlt)(HDC, int, int, int, int, HDC, int, int, DWORD);
PBitBlt OriginalBitBlt = NULL;

typedef BOOL(WINAPI* PStretchBlt)(HDC, int, int, int, int, HDC, int, int, int, int, DWORD);
PStretchBlt OriginalStretchBlt = NULL;


// Global handle to our decoy bitmap
HBITMAP g_hDecoyBitmap = NULL;
int g_decoyWidth = 0;
int g_decoyHeight = 0;

// Helper function to load a bitmap from a file
HBITMAP LoadDecoyBitmap(const WCHAR* filePath, int& width, int& height)
{
	// Use LoadImageW for wide characters
	HBITMAP hBitmap = (HBITMAP)LoadImageW(NULL, filePath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
	if (hBitmap)
	{
		BITMAP bitmapInfo;
		if (GetObject(hBitmap, sizeof(BITMAP), &bitmapInfo))
		{
			width = bitmapInfo.bmWidth;
			height = bitmapInfo.bmHeight;
		}
		cout << "[Hook Debug] Decoy bitmap loaded successfully. Path: " << filePath << ", Dims: " << width << "x" << height << endl;
	}
	else
	{
		wcerr << L"[*] Failed to load decoy bitmap: " << filePath << L", Error: " << GetLastError() << endl;
	}
	return hBitmap;
}

// Hook for BitBlt
BOOL WINAPI MyBitBltHook(
	HDC hdcDest,
	int xDest,
	int yDest,
	int wDest,
	int hDest,
	HDC hdcSrc,
	int xSrc,
	int ySrc,
	DWORD dwRop)
{
	cout << "[Hook Debug] BitBlt called. hdcSrc: " << hdcSrc << ", dwRop: " << hex << dwRop << dec << endl;
	cout << "[Hook Debug] wDest: " << wDest << ", hDest: " << hDest << endl;

	// Get the screen DC from within this process's context for comparison/GetDeviceCaps
	HDC currentScreenDCForCaps = GetDC(NULL);
	cout << "[Hook Debug] GetDC(NULL) in hook: " << currentScreenDCForCaps << endl;

	int screen_width = GetSystemMetrics(SM_CXSCREEN);
	int screen_height = GetSystemMetrics(SM_CYSCREEN);
	cout << "[Hook Debug] System Screen Dims: " << screen_width << "x" << screen_height << endl;

	// --- Detection Logic Refinement ---
	// Condition 1: Check if the source DC is a display device context (i.e., screen).
	// This is more robust than just comparing HDC handles directly.
	BOOL isSourceDisplayDC = FALSE;
	if (hdcSrc) {
		int technology = GetDeviceCaps(hdcSrc, TECHNOLOGY);
		if (technology == DT_RASDISPLAY) { // DT_RASDISPLAY indicates a raster display device
			isSourceDisplayDC = TRUE;
			cout << "[Hook Debug] hdcSrc identified as a display DC (TECHNOLOGY == DT_RASDISPLAY)." << endl;
		}
		else {
			cout << "[Hook Debug] hdcSrc is NOT a display DC (TECHNOLOGY: " << technology << ")." << endl;
		}
	}
	else {
		cout << "[Hook Debug] hdcSrc is NULL." << endl;
	}

	// Condition 2: Check if the ROP code is SRCCOPY (typical for direct copies/screenshots).
	BOOL isSrcCopyRop = (dwRop == SRCCOPY);
	if (!isSrcCopyRop) {
		cout << "[Hook Debug] dwRop is NOT SRCCOPY (dwRop: " << hex << dwRop << dec << ")." << endl;
	}

	// Condition 3 (Optional, but good for this PoC): Check if the destination dimensions match screen dimensions.
	// The malware PoC captures the full screen. If it were capturing partial screens, this check would fail.
	// For broader deception, you might remove this part if the malware captures smaller regions.
	BOOL isFullScreenCapture = (wDest == screen_width && hDest == screen_height);
	if (!isFullScreenCapture) {
		cout << "[Hook Debug] Destination dimensions (" << wDest << "x" << hDest << ") do NOT match full screen dimensions." << endl;
	}

	// Release the DC obtained for caps after use
	if (currentScreenDCForCaps) {
		ReleaseDC(NULL, currentScreenDCForCaps);
		currentScreenDCForCaps = NULL; // Set to NULL to avoid double release/use after release
	}


	if (isSourceDisplayDC && isSrcCopyRop && isFullScreenCapture) // All conditions for deception met
	{
		cout << "[Hook] BitBlt intercepted: Suspected full screen capture from process " << GetCurrentProcessId() << "!" << endl;
		cout << "[Hook] All conditions met for deception." << endl;

		if (g_hDecoyBitmap && g_decoyWidth > 0 && g_decoyHeight > 0)
		{
			HDC hMemDC = CreateCompatibleDC(hdcDest);
			if (hMemDC)
			{
				HGDIOBJ hOldBitmap = SelectObject(hMemDC, g_hDecoyBitmap);

				BOOL result = FALSE;
				if (OriginalStretchBlt) // Prefer StretchBlt for proper scaling
				{
					result = OriginalStretchBlt(hdcDest, xDest, yDest, wDest, hDest,
						hMemDC, 0, 0, g_decoyWidth, g_decoyHeight, SRCCOPY);
					cout << "[Deception] Used StretchBlt to copy decoy." << endl;
				}
				else // Fallback to BitBlt if StretchBlt not found (less ideal for scaling)
				{
					// BitBlt won't scale. It will only copy the top-left portion of the decoy
					// if it's smaller than the destination, or fill the destination if the decoy is larger.
					result = OriginalBitBlt(hdcDest, xDest, yDest, wDest, hDest,
						hMemDC, 0, 0, SRCCOPY);
					cout << "[Deception] Used BitBlt to copy decoy (StretchBlt not found or preferred)." << endl;
				}

				SelectObject(hMemDC, hOldBitmap);
				DeleteDC(hMemDC);

				cout << "[Deception] Replaced screen content with decoy image. Result: " << (result ? "TRUE" : "FALSE") << endl;
				return result; // Return TRUE if deception was successful
			}
			else
			{
				cerr << "[Deception] Failed to create compatible DC for decoy. Falling back to original BitBlt. Error: " << GetLastError() << endl;
			}
		}
		else
		{
			cerr << "[Deception] Decoy image not loaded or invalid (" << g_hDecoyBitmap << ", " << g_decoyWidth << ", " << g_decoyHeight << "). Falling back to original BitBlt." << endl;
		}
	}
	else {
		cout << "[Hook Debug] BitBlt conditions not met for deception. Passing to original." << endl;
	}

	// For all other BitBlt calls, or if our deception failed, call the original function
	return OriginalBitBlt(hdcDest, xDest, yDest, wDest, hDest, hdcSrc, xSrc, ySrc, dwRop);
}

// Random generator (from sample)
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dist(0, 99);

// Hook: CreateFileW (from sample)
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

// Deceptive GetAsyncKeyState with selective printing (from sample)
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

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
	cout << "[*] Injection started." << endl;

	// Load the decoy bitmap when the DLL is injected
	g_hDecoyBitmap = LoadDecoyBitmap(L"decoy.bmp", g_decoyWidth, g_decoyHeight);
	if (g_hDecoyBitmap)
	{
		cout << "[*] Decoy bitmap 'decoy.bmp' loaded successfully. Dimensions: " << g_decoyWidth << "x" << g_decoyHeight << endl;
	}
	else
	{
		cerr << "[!] Failed to load decoy bitmap. Screenshot deception will not work." << endl;
	}

	HOOK_TRACE_INFO hBitBltHook = { NULL };
	HOOK_TRACE_INFO hCreateFileHook = { NULL };
	HOOK_TRACE_INFO hAsyncKeyHook = { NULL };

	// Install BitBlt hook
	OriginalBitBlt = (PBitBlt)GetProcAddress(GetModuleHandle(TEXT("gdi32")), "BitBlt");
	if (OriginalBitBlt == NULL) {
		OriginalBitBlt = (PBitBlt)GetProcAddress(GetModuleHandle(TEXT("gdi32full")), "BitBlt"); // Try gdi32full on newer Windows
	}

	// Get StretchBlt address as well
	OriginalStretchBlt = (PStretchBlt)GetProcAddress(GetModuleHandle(TEXT("gdi32")), "StretchBlt");
	if (OriginalStretchBlt == NULL) {
		OriginalStretchBlt = (PStretchBlt)GetProcAddress(GetModuleHandle(TEXT("gdi32full")), "StretchBlt");
	}


	if (OriginalBitBlt) {
		cout << "BitBlt function address: " << (void*)OriginalBitBlt << endl;
		NTSTATUS result = LhInstallHook(OriginalBitBlt, MyBitBltHook, nullptr, &hBitBltHook);
		if (FAILED(result)) {
			wstring s(RtlGetLastErrorString());
			wcerr << L"Failed to install BitBlt hook: " << s << endl;
		}
		else {
			cout << "BitBlt hook installed successfully!" << endl;
		}
	}
	else {
		cerr << "Failed to find BitBlt function address!" << endl;
	}

	if (OriginalStretchBlt) {
		cout << "StretchBlt function address: " << (void*)OriginalStretchBlt << endl;
	}
	else {
		cerr << "Failed to find StretchBlt function address! Decoy scaling might be impacted." << endl;
	}

	// Install other hooks (from previous samples)
	FARPROC createFileAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "CreateFileW");
	LhInstallHook(createFileAddr, myCreateFileWHook, nullptr, &hCreateFileHook);

	FARPROC asyncKeyAddr = GetProcAddress(GetModuleHandle(TEXT("user32")), "GetAsyncKeyState");
	LhInstallHook(asyncKeyAddr, myGetAsyncKeyStateHook, nullptr, &hAsyncKeyHook);

	// Enable all hooks
	ULONG ACLEntries[1] = { 0 };
	LhSetExclusiveACL(ACLEntries, 1, &hBitBltHook);
	LhSetExclusiveACL(ACLEntries, 1, &hCreateFileHook);
	LhSetExclusiveACL(ACLEntries, 1, &hAsyncKeyHook);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hModule);
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		if (g_hDecoyBitmap)
		{
			DeleteObject(g_hDecoyBitmap);
			g_hDecoyBitmap = NULL;
		}
		// Clean up EasyHook resources on process detach
		// LhUninstallAllHooks(); // Consider if you want to explicitly uninstall
		// RtlFinalizeEasyHook(); // Not usually necessary unless specifically initialized via RtlInitializeEasyHook
		break;
	}
	return TRUE;
}