#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <easyhook.h>

#pragma comment(lib, "EasyHook32.lib") // Change to EasyHook64.lib if building x64

using namespace std;

// Typedef for tracking the original function signature
typedef BOOL(WINAPI* BITBLT_FUNC)(HDC, int, int, int, int, HDC, int, int, DWORD);

// Store pointer to the original BitBlt fallback trampoline
BITBLT_FUNC pfnOriginalBitBlt = nullptr;

// Hook function for BitBlt API
BOOL WINAPI myBitBltHook(
    HDC hdcDest,      // Destination device context (the malware's memory DC)
    int xDest,        // X coordinate of destination upper-left corner
    int yDest,        // Y coordinate of destination upper-left corner
    int width,        // Width of destination rectangle
    int height,       // Height of destination rectangle
    HDC hdcSrc,       // Source device context (the Desktop's DC)
    int xSrc,         // X coordinate of source upper-left corner
    int ySrc,         // Y coordinate of source upper-left corner
    DWORD rop)        // Raster operation code
{
    cout << "[Hook] BitBlt intercepted!" << endl;
    cout << "       Dimensions: " << width << "x" << height << " | ROP: 0x" << hex << rop << dec << endl;

    // Detect if the malware is copying from the desktop screen window
    HWND hSrcWindow = WindowFromDC(hdcSrc);
    HWND hDesktopWindow = GetDesktopWindow();

    if (hSrcWindow == hDesktopWindow || hdcSrc == GetDC(NULL))
    {
        cout << "[Deception] Desktop capture signature detected. Fabricating decoy screenshot..." << endl;

        // Step 1: Let the original BitBlt fill the surface structure sizes first
        BOOL result = BitBlt(hdcDest, xDest, yDest, width, height, hdcSrc, xSrc, ySrc, rop);

        // Step 2: Overwrite the buffer inside the malware's hdcDest with a benign decoy image
        // We will paint a neutral background color and draw a crosshair or text
        HBRUSH hDecoyBackground = CreateSolidBrush(RGB(240, 240, 240)); // Plain light gray
        RECT rectDest = { xDest, yDest, xDest + width, yDest + height };
        FillRect(hdcDest, &rectDest, hDecoyBackground);
        DeleteObject(hDecoyBackground);

        // Step 3: Draw decorative elements onto the decoy frame to make it look like a generic target
        HFONT hFont = CreateFontA(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Arial");
        HGDIOBJ hOldFont = SelectObject(hdcDest, hFont);

        SetTextColor(hdcDest, RGB(180, 50, 50));
        SetBkMode(hdcDest, TRANSPARENT);

        string message = "DECOY ENVIRONMENT ACTIVE - SECURE SANDBOX";
        DrawTextA(hdcDest, message.c_str(), -1, &rectDest, DT_CENTER | DT_SINGLELINE | DT_VCENTER);

        // Restore context structures
        SelectObject(hdcDest, hOldFont);
        DeleteObject(hFont);

        return TRUE; // Force success status back to malware engine
    }

    // Fall back to normal behavior if the app is just double-buffering its own inner interface elements
    return BitBlt(hdcDest, xDest, yDest, width, height, hdcSrc, xSrc, ySrc, rop);
}

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
    cout << "[*] Injection started inside target process." << endl;

    HOOK_TRACE_INFO hBitBltHook = { NULL };

    // Resolve address of BitBlt from GDI32 engine
    FARPROC bitBltAddr = GetProcAddress(GetModuleHandle(TEXT("gdi32.dll")), "BitBlt");
    if (bitBltAddr == nullptr) {
        cerr << "[-] Failed to locate BitBlt endpoint address." << endl;
        return;
    }

    // Install runtime interception hook
    NTSTATUS result = LhInstallHook(
        bitBltAddr,
        myBitBltHook,
        nullptr,
        &hBitBltHook
    );

    if (FAILED(result))
    {
        wstring s(RtlGetLastErrorString());
        wcerr << L"[-] Failed to install BitBlt redirection hook: " << s << endl;
        return;
    }
    else
    {
        cout << "[+] BitBlt redirection hook successfully initialized." << endl;
    }

    // Enable the hook for the current thread and process context exclusively
    ULONG ACLEntries[1] = { 0 };
    LhSetExclusiveACL(ACLEntries, 1, &hBitBltHook);
}