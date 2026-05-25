#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <easyhook.h>
#include <chrono>
#include <thread>

#pragma comment(lib, "EasyHook32.lib") // Ensure matching architecture (32/64)

using namespace std;

// Hook function for the BitBlt API implementing FakeFailure
BOOL WINAPI myBitBltHook(
    HDC hdcDest,      // Destination device context
    int xDest,        // X coordinate of destination
    int yDest,        // Y coordinate of destination
    int width,        // Width of destination rectangle
    int height,       // Height of destination rectangle
    HDC hdcSrc,       // Source device context
    int xSrc,         // X coordinate of source
    int ySrc,         // Y coordinate of source
    DWORD rop)        // Raster operation code
{
    // Context Validation: Check if the source device context corresponds to the Desktop screen
    HWND hSrcWindow = WindowFromDC(hdcSrc);
    HWND hDesktopWindow = GetDesktopWindow();

    if (hSrcWindow == hDesktopWindow || hdcSrc == GetDC(NULL))
    {
        cout << "\n[Deception] BitBlt screenshot signature intercepted!" << endl;
        cout << "[Deception] Executing FakeFailure strategy. Blocking pixel copy..." << endl;

        // Timing Realism: A normal BitBlt transfer for a standard monitor configuration 
        // typically takes anywhere between 500 to 2500+ microseconds depending on hardware.
        // We simulate a ~300000 microsecond overhead so time-based anti-analysis checks pass.
        auto startSim = std::chrono::high_resolution_clock::now();
        while (true) {
            auto now = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - startSim).count();
            if (elapsed >= 300000) break;
        }

        // Return TRUE to trick the malware's 'if (!BitBlt(...))' validation into thinking it worked.
        // The destination bitmap (hdcDest) remains unpopulated/blank.
        return TRUE;
    }

    // Ensure legitimate, non-desktop related internal drawing requests continue to work properly
    return BitBlt(hdcDest, xDest, yDest, width, height, hdcSrc, xSrc, ySrc, rop);
}


extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
    cout << "[*] Deception DLL Injection started." << endl;

    HOOK_TRACE_INFO hBitBltHook = { NULL };

    // Resolve the address of BitBlt inside GDI32.dll
    FARPROC bitBltAddr = GetProcAddress(GetModuleHandle(TEXT("gdi32.dll")), "BitBlt");

    if (bitBltAddr == nullptr) {
        cerr << "[-] Failed to resolve BitBlt export endpoint address." << endl;
        return;
    }

    // Install runtime hooking interface via EasyHook
    NTSTATUS result = LhInstallHook(
        bitBltAddr,
        myBitBltHook,
        nullptr,
        &hBitBltHook
    );

    if (FAILED(result))
    {
        wstring s(RtlGetLastErrorString());
        wcerr << L"[-] Failed to install BitBlt hook: " << s << endl;
        return;
    }
    else
    {
        cout << "[+] BitBlt FakeFailure hook installed successfully." << endl;
    }

    // Configure Access Control List to apply strictly to this injected process context.
    // This leaves the system's legitimate background screenshot tools running cleanly.
    ULONG ACLEntries[1] = { 0 };
    LhSetExclusiveACL(ACLEntries, 1, &hBitBltHook);
}