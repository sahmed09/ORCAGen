// AntiScreenLoggerFakeFailureHook.cpp

#include "pch.h"
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <easyhook.h>

#pragma comment(lib, "EasyHook32.lib") // Use EasyHook64.lib for x64

using namespace std;

HOOK_TRACE_INFO gBitBltHook = { NULL };

bool IsLikelyMalwareScreenshot(
    HDC hdcSrc,
    int xSrc,
    int ySrc,
    int width,
    int height,
    DWORD rop)
{
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    bool usesScreenCopy =
        (rop & SRCCOPY) == SRCCOPY;

    bool usesCaptureBlt =
        (rop & CAPTUREBLT) == CAPTUREBLT;

    bool fullOrLargeScreenCapture =
        width >= screenW / 2 &&
        height >= screenH / 2;

    bool startsAtScreenOrigin =
        xSrc == 0 &&
        ySrc == 0;

    int srcW = GetDeviceCaps(hdcSrc, HORZRES);
    int srcH = GetDeviceCaps(hdcSrc, VERTRES);

    bool sourceLooksLikeScreen =
        srcW == screenW &&
        srcH == screenH;

    return usesScreenCopy &&
        usesCaptureBlt &&
        fullOrLargeScreenCapture &&
        startsAtScreenOrigin &&
        sourceLooksLikeScreen;
}

BOOL WINAPI myBitBltHook(
    HDC hdcDest,
    int xDest,
    int yDest,
    int width,
    int height,
    HDC hdcSrc,
    int xSrc,
    int ySrc,
    DWORD rop)
{
    if (IsLikelyMalwareScreenshot(hdcSrc, xSrc, ySrc, width, height, rop))
    {
        std::this_thread::sleep_for(std::chrono::microseconds(250000));
        cout << "[FakeFailure] Malware-like BitBlt screenshot attempt blocked." << endl;
        cout << "[FakeFailure] Screenshot was not captured." << endl;

        SetLastError(ERROR_ACCESS_DENIED);

        return FALSE;
    }

    return BitBlt(
        hdcDest,
        xDest,
        yDest,
        width,
        height,
        hdcSrc,
        xSrc,
        ySrc,
        rop
    );
}

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(
    REMOTE_ENTRY_INFO * inRemoteInfo)
{
    cout << "[*] Anti-screen-logger FakeFailure DLL injected." << endl;

    HMODULE gdi32 = GetModuleHandleW(L"gdi32.dll");

    if (gdi32 == NULL)
        gdi32 = LoadLibraryW(L"gdi32.dll");

    if (gdi32 == NULL)
    {
        cout << "[-] Failed to load gdi32.dll." << endl;
        return;
    }

    FARPROC bitBltAddr = GetProcAddress(gdi32, "BitBlt");

    if (bitBltAddr == NULL)
    {
        cout << "[-] Failed to resolve BitBlt." << endl;
        return;
    }

    NTSTATUS result = LhInstallHook(
        bitBltAddr,
        myBitBltHook,
        nullptr,
        &gBitBltHook
    );

    if (FAILED(result))
    {
        wcout << L"[-] Failed to install BitBlt hook: "
            << RtlGetLastErrorString()
            << endl;
        return;
    }

    ULONG ACLEntries[1] = { 0 };

    LhSetExclusiveACL(
        ACLEntries,
        1,
        &gBitBltHook
    );

    cout << "[+] BitBlt FakeFailure hook installed successfully." << endl;
}