#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <easyhook.h>
#include <random>

#pragma comment(lib, "EasyHook32.lib")  // or EasyHook64.lib depending on platform

using namespace std;

// Random generator for deception decisions
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dist(0, 99);
std::uniform_int_distribution<> colorDist(0, 255);

// Function to generate a decoy bitmap with various patterns
HBITMAP GenerateDecoyBitmap(HDC hdc, int width, int height)
{
    // Create compatible DC and bitmap
    HDC hMemDC = CreateCompatibleDC(hdc);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdc, width, height);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);

    // Choose random decoy pattern
    int patternType = dist(gen) % 4;

    switch (patternType)
    {
    case 0: // Solid color with text
    {
        RECT rect = { 0, 0, width, height };
        HBRUSH hBrush = CreateSolidBrush(RGB(colorDist(gen), colorDist(gen), colorDist(gen)));
        FillRect(hMemDC, &rect, hBrush);
        DeleteObject(hBrush);

        // Add warning text
        SetBkMode(hMemDC, TRANSPARENT);
        SetTextColor(hMemDC, RGB(255, 255, 255));
        HFONT hFont = CreateFont(40, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, TEXT("Arial"));
        HFONT hOldFont = (HFONT)SelectObject(hMemDC, hFont);

        const char* text = "DECOY SCREEN - NO DATA";
        TextOutA(hMemDC, width / 6, height / 2, text, strlen(text));

        SelectObject(hMemDC, hOldFont);
        DeleteObject(hFont);
        break;
    }

    case 1: // Gradient pattern
    {
        for (int y = 0; y < height; y++)
        {
            int r = (y * 255) / height;
            int g = ((height - y) * 255) / height;
            int b = 128;
            HBRUSH hBrush = CreateSolidBrush(RGB(r, g, b));
            RECT rect = { 0, y, width, y + 1 };
            FillRect(hMemDC, &rect, hBrush);
            DeleteObject(hBrush);
        }
        break;
    }

    case 2: // Checkerboard pattern
    {
        int tileSize = 64;
        for (int y = 0; y < height; y += tileSize)
        {
            for (int x = 0; x < width; x += tileSize)
            {
                COLORREF color = ((x / tileSize + y / tileSize) % 2 == 0) ?
                    RGB(220, 220, 220) : RGB(80, 80, 80);
                HBRUSH hBrush = CreateSolidBrush(color);
                RECT rect = { x, y, min(x + tileSize, width), min(y + tileSize, height) };
                FillRect(hMemDC, &rect, hBrush);
                DeleteObject(hBrush);
            }
        }
        break;
    }

    case 3: // Noise pattern
    {
        // White background
        RECT rect = { 0, 0, width, height };
        HBRUSH hBrush = CreateSolidBrush(RGB(245, 245, 245));
        FillRect(hMemDC, &rect, hBrush);
        DeleteObject(hBrush);

        // Add random noise
        for (int i = 0; i < (width * height) / 50; i++)
        {
            int x = dist(gen) % width;
            int y = dist(gen) % height;
            SetPixel(hMemDC, x, y, RGB(colorDist(gen), colorDist(gen), colorDist(gen)));
        }
        break;
    }
    }

    SelectObject(hMemDC, hOldBitmap);
    DeleteDC(hMemDC);

    return hBitmap;
}

// Hook: BitBlt - Primary screen capture interception
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
    // Detect potential screen capture operations
    // Criteria: SRCCOPY operation with significant dimensions
    bool isSuspiciousCapture = (rop == SRCCOPY) && (width >= 800 || height >= 600);

    if (isSuspiciousCapture)
    {
        // Apply deception 75% of the time, allow legitimate 25%
        int deceptionChance = dist(gen);

        if (deceptionChance < 100)
        {
            cout << "[+] BitBlt Hook Activated!" << endl;
            cout << "    Intercepted screen capture: " << width << "x" << height << endl;
            cout << "    Injecting decoy bitmap..." << endl;

            // Generate decoy bitmap
            HBITMAP hDecoyBitmap = GenerateDecoyBitmap(hdcDest, width, height);

            // Create memory DC for decoy
            HDC hDecoyDC = CreateCompatibleDC(hdcDest);
            HBITMAP hOldBitmap = (HBITMAP)SelectObject(hDecoyDC, hDecoyBitmap);

            // Copy decoy to destination
            BOOL result = BitBlt(hdcDest, xDest, yDest, width, height,
                hDecoyDC, 0, 0, SRCCOPY);

            // Cleanup
            SelectObject(hDecoyDC, hOldBitmap);
            DeleteObject(hDecoyBitmap);
            DeleteDC(hDecoyDC);

            cout << "    Decoy injection successful!" << endl;
            return result;
        }
        else
        {
            cout << "[*] BitBlt Hook: Allowing legitimate capture (25% pass-through)" << endl;
        }
    }

    // Call original BitBlt for non-suspicious or allowed operations
    return BitBlt(hdcDest, xDest, yDest, width, height, hdcSrc, xSrc, ySrc, rop);
}

// Hook: GetDC - Monitor screen DC requests
HDC WINAPI myGetDCHook(HWND hWnd)
{
    if (hWnd == NULL)
    {
        cout << "[*] GetDC Hook: Full screen DC requested (potential screen capture)" << endl;
    }
    return GetDC(hWnd);
}

// Hook: CreateCompatibleBitmap - Monitor large bitmap creation
HBITMAP WINAPI myCreateCompatibleBitmapHook(HDC hdc, int width, int height)
{
    if (width > 1024 || height > 768)
    {
        cout << "[*] CreateCompatibleBitmap Hook: Large bitmap detected - "
            << width << "x" << height << " (possible capture buffer)" << endl;
    }
    return CreateCompatibleBitmap(hdc, width, height);
}

// DLL Entry Point - Hook Installation
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
    cout << "================================================" << endl;
    cout << "[*] Anti-Screen Logger Hook Initialized" << endl;
    cout << "[*] Defensive Deception Framework Active" << endl;
    cout << "================================================" << endl;

    // Hook trace structures
    HOOK_TRACE_INFO hBitBltHook = { NULL };
    HOOK_TRACE_INFO hGetDCHook = { NULL };
    HOOK_TRACE_INFO hCreateBitmapHook = { NULL };

    // Get module handles
    HMODULE hGdi32 = GetModuleHandle(TEXT("gdi32"));
    HMODULE hUser32 = GetModuleHandle(TEXT("user32"));

    // Install BitBlt hook (primary screen capture API)
    FARPROC bitBltAddr = GetProcAddress(hGdi32, "BitBlt");
    if (bitBltAddr)
    {
        NTSTATUS result = LhInstallHook(bitBltAddr, myBitBltHook, nullptr, &hBitBltHook);
        if (FAILED(result))
        {
            wstring s(RtlGetLastErrorString());
            wcerr << L"[-] Failed to install BitBlt hook: " << s << endl;
        }
        else
        {
            cout << "[+] BitBlt hook installed successfully!" << endl;
        }
    }

    // Install GetDC hook (screen DC monitoring)
    FARPROC getDCAddr = GetProcAddress(hUser32, "GetDC");
    if (getDCAddr)
    {
        NTSTATUS result = LhInstallHook(getDCAddr, myGetDCHook, nullptr, &hGetDCHook);
        if (FAILED(result))
        {
            wstring s(RtlGetLastErrorString());
            wcerr << L"[-] Failed to install GetDC hook: " << s << endl;
        }
        else
        {
            cout << "[+] GetDC hook installed successfully!" << endl;
        }
    }

    // Install CreateCompatibleBitmap hook (bitmap creation monitoring)
    FARPROC createBitmapAddr = GetProcAddress(hGdi32, "CreateCompatibleBitmap");
    if (createBitmapAddr)
    {
        NTSTATUS result = LhInstallHook(createBitmapAddr, myCreateCompatibleBitmapHook, nullptr, &hCreateBitmapHook);
        if (FAILED(result))
        {
            wstring s(RtlGetLastErrorString());
            wcerr << L"[-] Failed to install CreateCompatibleBitmap hook: " << s << endl;
        }
        else
        {
            cout << "[+] CreateCompatibleBitmap hook installed successfully!" << endl;
        }
    }

    // Enable hooks for all threads
    ULONG ACLEntries[1] = { 0 };
    LhSetExclusiveACL(ACLEntries, 1, &hBitBltHook);
    LhSetExclusiveACL(ACLEntries, 1, &hGetDCHook);
    LhSetExclusiveACL(ACLEntries, 1, &hCreateBitmapHook);

    cout << "[*] All hooks activated and monitoring..." << endl;
    cout << "[*] Screen capture deception ready!" << endl;
    cout << "================================================" << endl;
}