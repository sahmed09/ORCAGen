#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <easyhook.h>
#include <random>

#pragma comment(lib, "EasyHook32.lib") // Link with EasyHook32.lib

using namespace std;

// Global variable to store the original BitBlt function pointer.
// This is crucial for calling the legitimate BitBlt function when not deceiving.
typedef BOOL(WINAPI* BitBlt_t)(HDC, int, int, int, int, HDC, int, int, DWORD);
BitBlt_t OriginalBitBlt = NULL;

// Random generator for deceptive GetAsyncKeyState (already present in sample)
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dist(0, 99);

// Hook function for CreateFileW (from sample, for reference)
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

	// Call the original CreateFileW function
	return CreateFileW(lpFileName,
		dwDesiredAccess,
		dwShareMode,
		lpSecurityAttributes,
		dwCreationDisposition,
		dwFlagsAndAttributes,
		hTemplateFile);
}

// Deceptive GetAsyncKeyState with selective printing (from sample, for reference)
SHORT WINAPI myGetAsyncKeyStateHook(int vKey)
{
	SHORT actualState = GetAsyncKeyState(vKey);

	int chance = dist(gen);
	if (chance < 20) // 20% chance to flip the key pressed bit
	{
		actualState ^= 0x8000;  // Flip the highest bit (key pressed state)
		cout << "[Deception] Modified GetAsyncKeyState for VK: " << vKey << endl;
	}

	return actualState;
}

// NEW: Hook function for BitBlt API
// This function will intercept calls to BitBlt and implement the deception logic.
BOOL WINAPI myBitBltHook(HDC hdcDest, int xDest, int yDest, int wDest, int hDest, HDC hdcSrc, int xSrc, int ySrc, DWORD dwRop)
{
	cout << "[+] BitBltHook triggered!" << endl;

	// Heuristic to detect screenshot attempts:
	// 1. Check if the source device context (hdcSrc) is a display device context.
	//    GetDeviceCaps(hdcSrc, TECHNOLOGY) == DT_RASDISPLAY indicates a raster display device.
	// 2. Check if the raster operation (dwRop) is SRCCOPY, which means a direct copy of pixels.
	// The malware's screenshot code uses GetDC(NULL) for hdcScreen and SRCCOPY.
	// This combination strongly suggests a screen capture operation.
	if (GetDeviceCaps(hdcSrc, TECHNOLOGY) == DT_RASDISPLAY && dwRop == SRCCOPY)
	{
		cout << "[Deception] Intercepted potential screenshot attempt via BitBlt!" << endl;
		cout << "[Deception] Returning FakeFailure (FALSE) to mislead malware." << endl;
		// Return FALSE to simulate a failure, preventing the screenshot from being captured correctly.
		// The malware's CaptureScreenAndSave function will then likely fail to save the bitmap.
		return FALSE;
	}

	// For any other legitimate BitBlt calls (e.g., drawing to a window, copying from memory DC),
	// call the original BitBlt function to ensure normal application functionality.
	cout << "[Hook] Legitimate BitBlt call - forwarding to original." << endl;
	return OriginalBitBlt(hdcDest, xDest, yDest, wDest, hDest, hdcSrc, xSrc, ySrc, dwRop);
}


// Entry point for the injected DLL
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO* inRemoteInfo)
{
	cout << "[*] Injection started." << endl;

	// Initialize HOOK_TRACE_INFO structures for each hook
	HOOK_TRACE_INFO hCreateFileHook = { NULL };
	HOOK_TRACE_INFO hAsyncKeyHook = { NULL };
	HOOK_TRACE_INFO hBitBltHook = { NULL }; // New hook info for BitBlt

	// --- Install CreateFileW hook ---
	// Get the address of the original CreateFileW function from kernel32.dll
	FARPROC createFileAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "CreateFileW");
	if (createFileAddr) {
		// Install the hook: redirect calls from createFileAddr to myCreateFileWHook
		LhInstallHook(createFileAddr, myCreateFileWHook, nullptr, &hCreateFileHook);
		cout << "CreateFileW hook installed." << endl;
	}
	else {
		// Log error if function address cannot be obtained
		wcerr << L"Failed to get address for CreateFileW." << endl;
	}

	// --- Install GetAsyncKeyState hook ---
	// Get the address of the original GetAsyncKeyState function from user32.dll
	FARPROC asyncKeyAddr = GetProcAddress(GetModuleHandle(TEXT("user32")), "GetAsyncKeyState");
	if (asyncKeyAddr) {
		// Install the hook: redirect calls from asyncKeyAddr to myGetAsyncKeyStateHook
		LhInstallHook(asyncKeyAddr, myGetAsyncKeyStateHook, nullptr, &hAsyncKeyHook);
		cout << "GetAsyncKeyState hook installed." << endl;
	}
	else {
		// Log error if function address cannot be obtained
		wcerr << L"Failed to get address for GetAsyncKeyState." << endl;
	}

	// --- NEW: Install BitBlt hook ---
	// Get the address of the original BitBlt function from gdi32.dll
	FARPROC bitBltAddr = GetProcAddress(GetModuleHandle(TEXT("gdi32")), "BitBlt");
	if (bitBltAddr) {
		// Store the original function address in our global pointer
		OriginalBitBlt = (BitBlt_t)bitBltAddr;
		// Install the hook: redirect calls from bitBltAddr to myBitBltHook
		NTSTATUS result = LhInstallHook(bitBltAddr, myBitBltHook, nullptr, &hBitBltHook);

		if (FAILED(result))
		{
			// Log error if hook installation fails
			wstring s(RtlGetLastErrorString());
			wcerr << L"Failed to install BitBlt hook: " << s << endl;
		}
		else
		{
			cout << "BitBlt hook installed successfully!" << endl;
		}
	}
	else {
		// Log error if function address cannot be obtained
		wcerr << L"Failed to get address for BitBlt." << endl;
	}

	// --- Enable all installed hooks ---
	// Create an array of hook handles to enable them exclusively.
	// The size of the array should match the number of hooks being enabled.
	ULONG ACLEntries[3] = { 0 }; // Increased size to 3 for CreateFileW, GetAsyncKeyState, and BitBlt

	// Enable the hooks. LhSetExclusiveACL ensures that these hooks are the only ones active
	// for the specified threads (0 means all threads in the target process).
	LhSetExclusiveACL(ACLEntries, 3, &hCreateFileHook);
	LhSetExclusiveACL(ACLEntries, 3, &hAsyncKeyHook);
	LhSetExclusiveACL(ACLEntries, 3, &hBitBltHook); // Enable the new BitBlt hook

	cout << "[*] Injection finished. Hooks are active." << endl;
	// The DLL will remain loaded and the hooks active as long as the target process
	// (ClipboardLoggerPOC.exe) is running and the injector (InjectAntiClipboardHook.exe)
	// keeps the remote process alive (which it does by waiting for user input).
}
