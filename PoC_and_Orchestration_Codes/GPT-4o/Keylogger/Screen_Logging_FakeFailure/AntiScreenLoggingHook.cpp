#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#include <iostream>
#include <string>
#include <easyhook.h>

#pragma comment(lib, "EasyHook32.lib") // Or EasyHook64.lib for 64-bit
#pragma comment(lib, "Psapi.lib")      // For GetModuleFileNameExW

using namespace std;

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

// Global pointer to original BitBlt function
BitBltFunc TrueBitBlt = nullptr;

// Hook info
HOOK_TRACE_INFO hBitBltHook = { NULL };

// Check if current process is malicious
bool IsMaliciousProcess()
{
	WCHAR processPath[MAX_PATH] = { 0 };

	// Use Unicode version
	if (GetModuleFileNameExW(GetCurrentProcess(), NULL, processPath, MAX_PATH))
	{
		std::wstring exeName = std::wstring(processPath);

		// Match malware executable name
		if (exeName.find(L"ClipboardLoggerPOC") != std::wstring::npos)
		{
			std::wcout << L"[Deception] Malicious process detected: " << exeName << std::endl;
			return true;
		}
	}

	return false;
}

// BitBlt hook implementation
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
	if (IsMaliciousProcess())
	{
		std::wcout << L"[Hook] BitBlt intercepted from malicious process. Blocking screen capture." << std::endl;

		// Return FALSE (fake failure)
		return FALSE;
	}

	// Allow legitimate screen capture
	return TrueBitBlt(hdcDest, nXDest, nYDest, nWidth, nHeight,
		hdcSrc, nXSrc, nYSrc, dwRop);
}

// EasyHook DLL entry point
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO* inRemoteInfo)
{
	std::wcout << L"[*] Injection started." << std::endl;

	// Resolve BitBlt address
	FARPROC bitBltAddr = GetProcAddress(GetModuleHandle(TEXT("gdi32")), "BitBlt");
	TrueBitBlt = (BitBltFunc)bitBltAddr;

	if (bitBltAddr == nullptr)
	{
		std::wcerr << L"[!] Failed to resolve BitBlt address." << std::endl;
		return;
	}

	// Install hook
	NTSTATUS result = LhInstallHook(
		bitBltAddr,
		myBitBltHook,
		nullptr,
		&hBitBltHook);

	if (FAILED(result))
	{
		std::wcerr << L"[!] Failed to install BitBlt hook: " << RtlGetLastErrorString() << std::endl;
		return;
	}

	std::wcout << L"[+] BitBlt hook installed successfully!" << std::endl;

	// Enable hook for all threads in current process
	ULONG ACLEntries[1] = { 0 };
	LhSetExclusiveACL(ACLEntries, 1, &hBitBltHook);
}
