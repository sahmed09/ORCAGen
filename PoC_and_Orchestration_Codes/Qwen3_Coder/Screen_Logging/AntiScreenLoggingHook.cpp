// File: ScreenCaptureHook.cpp
#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <easyhook.h>

#pragma comment(lib, "EasyHook32.lib")

using namespace std;

// Global random generator for decoy image
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dist(0, 255);

// Function to generate a fake BMP file in memory
vector<BYTE> GenerateFakeBMP(int width, int height)
{
    DWORD dwBmpSize = ((width * 32 + 31) / 32) * 4 * height;
    BITMAPINFOHEADER bi = { 0 };
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = width;
    bi.biHeight = -height;
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;

    BITMAPFILEHEADER bf = { 0 };
    bf.bfType = 0x4D42; // "BM"
    bf.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwBmpSize;
    bf.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    vector<BYTE> bmpData(dwBmpSize);
    for (DWORD i = 0; i < dwBmpSize; ++i)
    {
        bmpData[i] = static_cast<BYTE>(dist(gen));
    }

    vector<BYTE> result;
    result.resize(bf.bfSize);

    memcpy(result.data(), &bf, sizeof(BITMAPFILEHEADER));
    memcpy(result.data() + sizeof(BITMAPFILEHEADER), &bi, sizeof(BITMAPINFOHEADER));
    memcpy(result.data() + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER), bmpData.data(), dwBmpSize);

    return result;
}

// Original BitBlt function pointer
typedef BOOL(WINAPI* tBitBlt)(
    HDC hdcDest,
    int nXDest,
    int nYDest,
    int nWidth,
    int nHeight,
    HDC hdcSrc,
    int nXSrc,
    int nYSrc,
    DWORD dwRop);

tBitBlt RealBitBlt = nullptr;

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
    // Get current process ID
    DWORD pid = GetCurrentProcessId();

    // Detect if this is the malicious process (replace with actual PID or detection logic)
    // if (pid == 12345) // <-- Replace this with real PID of malware
    // {
        cout << "[Deception] Intercepted BitBlt from malware process." << endl;

        // Generate fake image data
        auto fakeBmp = GenerateFakeBMP(nWidth, nHeight);
        if (fakeBmp.empty()) return FALSE;

        // Write fake bitmap to destination DC (simulate a valid capture)
        // Note: This approach assumes hdcDest supports writing back into it.
        // A more advanced version could draw directly on screen or simulate memory access.

        // For now, just return success without modifying anything visually
        // because we don't want to corrupt user's actual display
        return TRUE;
    // }

    // Let normal BitBlt continue
    return RealBitBlt(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, dwRop);
}

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
    cout << "[*] Injection started." << endl;

    HOOK_TRACE_INFO hBitBltHook = { NULL };

    // Get address of real BitBlt
    FARPROC bitBltAddr = GetProcAddress(GetModuleHandle(TEXT("gdi32")), "BitBlt");
    if (!bitBltAddr)
    {
        cout << "[Error] Failed to get BitBlt address." << endl;
        return;
    }

    // Install hook
    LhInstallHook(bitBltAddr, myBitBltHook, nullptr, &hBitBltHook);

    // Enable the hook
    ULONG ACLEntries[1] = { 0 };
    LhSetExclusiveACL(ACLEntries, 1, &hBitBltHook);

    // Store real function pointer for later use
    RealBitBlt = (tBitBlt)bitBltAddr;
}