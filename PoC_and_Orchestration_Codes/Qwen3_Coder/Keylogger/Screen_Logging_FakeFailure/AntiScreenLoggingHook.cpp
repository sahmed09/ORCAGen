#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <random>
#include <easyhook.h>
#include <vector>
#include <thread>
#include <chrono>

#pragma comment(lib, "EasyHook32.lib")

using namespace std;

// Global variables for tracking hooked functions
HOOK_TRACE_INFO hBitBltHook = { NULL };
HOOK_TRACE_INFO hGetDIBitsHook = { NULL };

// Hook: BitBlt - always block
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
    cout << "[Deception] BitBlt intercepted - screenshot blocked" << endl;
    Sleep(1000);
    return FALSE; // Always block
}

// Hook: GetDIBits - always block
int WINAPI myGetDIBitsHook(
    HDC hdc,
    HBITMAP hbit,
    UINT uStartScan,
    UINT cScanLines,
    LPVOID lpvBits,
    LPBITMAPINFO lpbmi,
    UINT uUsage)
{
    cout << "[Deception] GetDIBits intercepted - screenshot blocked" << endl;
    Sleep(1000);
    return 0; // Always block
}

// Function to inject hooks into target process
extern "C" __declspec(dllexport) void __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
    cout << "[*] Injection started for screen capture defense." << endl;

    FARPROC bitBltAddr = GetProcAddress(GetModuleHandle(TEXT("gdi32")), "BitBlt");
    if (bitBltAddr != NULL) {
        if (LhInstallHook(bitBltAddr, myBitBltHook, nullptr, &hBitBltHook) == 0) {
            cout << "[+] Hook installed on BitBlt" << endl;
            ULONG ACLEntries[1] = { 0 };
            LhSetExclusiveACL(ACLEntries, 1, &hBitBltHook);
        }
        else {
            cout << "[-] Failed to hook BitBlt" << endl;
        }
    }

    FARPROC getDIBitsAddr = GetProcAddress(GetModuleHandle(TEXT("gdi32")), "GetDIBits");
    if (getDIBitsAddr != NULL) {
        if (LhInstallHook(getDIBitsAddr, myGetDIBitsHook, nullptr, &hGetDIBitsHook) == 0) {
            cout << "[+] Hook installed on GetDIBits" << endl;
            ULONG ACLEntries[1] = { 0 };
            LhSetExclusiveACL(ACLEntries, 1, &hGetDIBitsHook);
        }
        else {
            cout << "[-] Failed to hook GetDIBits" << endl;
        }
    }

    cout << "[+] All hooks installed and enabled." << endl;
}
