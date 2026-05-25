// AntiScreenLoggerHook.cpp

#include "pch.h"
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <easyhook.h>

#pragma comment(lib, "EasyHook32.lib") // Use EasyHook64.lib for x64 builds

using namespace std;

HOOK_TRACE_INFO gBitBltHook = { NULL };

bool IsLikelyScreenCapture(
    HDC hdcSrc,
    int xSrc,
    int ySrc,
    int width,
    int height,
    DWORD rop)
{
    if ((rop & SRCCOPY) != SRCCOPY)
        return false;

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    bool largeCapture =
        width >= screenW / 2 &&
        height >= screenH / 2;

    bool originCapture =
        xSrc == 0 &&
        ySrc == 0;

    int srcW = GetDeviceCaps(hdcSrc, HORZRES);
    int srcH = GetDeviceCaps(hdcSrc, VERTRES);

    bool sourceLooksLikeScreen =
        srcW == screenW &&
        srcH == screenH;

    bool usesCaptureBlt =
        (rop & CAPTUREBLT) == CAPTUREBLT;

    return largeCapture && originCapture && sourceLooksLikeScreen && usesCaptureBlt;
}

void DrawDecoyScreen(HDC hdcDest, int xDest, int yDest, int width, int height)
{
    HBRUSH backgroundBrush = CreateSolidBrush(RGB(25, 25, 25));
    RECT rect = { xDest, yDest, xDest + width, yDest + height };

    FillRect(hdcDest, &rect, backgroundBrush);
    DeleteObject(backgroundBrush);

    HPEN gridPen = CreatePen(PS_SOLID, 1, RGB(80, 80, 80));
    HGDIOBJ oldPen = SelectObject(hdcDest, gridPen);

    for (int x = xDest; x < xDest + width; x += 100)
    {
        MoveToEx(hdcDest, x, yDest, nullptr);
        LineTo(hdcDest, x, yDest + height);
    }

    for (int y = yDest; y < yDest + height; y += 100)
    {
        MoveToEx(hdcDest, xDest, y, nullptr);
        LineTo(hdcDest, xDest + width, y);
    }

    SelectObject(hdcDest, oldPen);
    DeleteObject(gridPen);

    SetBkMode(hdcDest, TRANSPARENT);
    SetTextColor(hdcDest, RGB(0, 255, 128));

    const wchar_t* msg1 = L"DECOY SCREENSHOT";
    const wchar_t* msg2 = L"Cyber Deception Active";
    const wchar_t* msg3 = L"Real screen content was not exposed";

    HFONT font = CreateFontW(
        36, 0, 0, 0,
        FW_BOLD,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_SWISS,
        L"Consolas"
    );

    HGDIOBJ oldFont = SelectObject(hdcDest, font);

    TextOutW(hdcDest, xDest + 80, yDest + 100, msg1, lstrlenW(msg1));
    TextOutW(hdcDest, xDest + 80, yDest + 160, msg2, lstrlenW(msg2));
    TextOutW(hdcDest, xDest + 80, yDest + 220, msg3, lstrlenW(msg3));

    SelectObject(hdcDest, oldFont);
    DeleteObject(font);
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
    if (IsLikelyScreenCapture(hdcSrc, xSrc, ySrc, width, height, rop))
    {
        std::this_thread::sleep_for(std::chrono::microseconds(250000));

        cout << "[Deception] BitBlt screen capture intercepted." << endl;
        cout << "[Deception] Replacing real screen with decoy image." << endl;

        DrawDecoyScreen(hdcDest, xDest, yDest, width, height);

        SetLastError(ERROR_SUCCESS);
        return TRUE;
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
    cout << "[*] Anti-screen-logger hook DLL injected." << endl;

    HMODULE gdi32 = GetModuleHandleW(L"gdi32.dll");

    if (gdi32 == NULL)
    {
        gdi32 = LoadLibraryW(L"gdi32.dll");
    }

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

    cout << "[*] BitBlt address: " << bitBltAddr << endl;

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

    cout << "[+] BitBlt hook installed successfully." << endl;
}