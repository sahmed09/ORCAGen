#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <easyhook.h>

#pragma comment(lib, "EasyHook32.lib") // or EasyHook64.lib if building x64

using namespace std;

// Original function pointer
typedef HANDLE(WINAPI* GetClipboardDataPrototype)(UINT uFormat);
GetClipboardDataPrototype originalGetClipboardData = nullptr;

// Hooked GetClipboardData function
HANDLE WINAPI myGetClipboardDataHook(UINT uFormat)
{
	if (uFormat == CF_TEXT)
	{
		std::cout << "[Hooked] Intercepted GetClipboardData call. Returning decoy data.\n";

		const char* decoy = "DecoyPassword123!";
		size_t len = strlen(decoy) + 1;

		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
		if (hMem)
		{
			void* ptr = GlobalLock(hMem);
			if (ptr)
			{
				memcpy(ptr, decoy, len);
				GlobalUnlock(hMem);
				return hMem;
			}
			GlobalFree(hMem);
		}

		std::cerr << "[Hooked] Failed to allocate decoy clipboard data.\n";
		return nullptr;
	}

	// Pass through for other formats
	return originalGetClipboardData(uFormat);
}

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO* inRemoteInfo)
{
	std::cout << "[*] Injection started.\n";

	HOOK_TRACE_INFO hGetClipboardDataHook = { NULL };

	// Get the address of the target API
	FARPROC targetFunc = GetProcAddress(GetModuleHandleA("user32.dll"), "GetClipboardData");

	if (targetFunc == nullptr)
	{
		std::cerr << "[-] Failed to locate GetClipboardData.\n";
		return;
	}

	// Save original function pointer
	originalGetClipboardData = (GetClipboardDataPrototype)targetFunc;

	// Install the hook
	NTSTATUS result = LhInstallHook(
		targetFunc,
		myGetClipboardDataHook,
		nullptr,
		&hGetClipboardDataHook
	);

	if (FAILED(result))
	{
		std::wcerr << L"[-] Hook installation failed: " << RtlGetLastErrorString() << endl;
		return;
	}

	std::cout << "[+] GetClipboardData hook installed successfully.\n";

	// Set ACL so only the malware process is affected
	ULONG exclusiveACL[1] = { 0 }; // ThreadId 0 = current thread = target
	LhSetExclusiveACL(exclusiveACL, 1, &hGetClipboardDataHook);
}
