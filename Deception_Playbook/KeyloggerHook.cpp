// Combined Anti-Keylogger DLL
// Techniques included:
//   [A] Clipboard logger deception (OpenClipboard / GetClipboardData / SetClipboardData)
//   [B] Screen logger deception (BitBlt / PrintWindow)
//   [C] Mouse tracking deception (GetCursorPos)
//   [D] Keyboard deception + API name logging (GetAsyncKeyState / GetKeyState / ToAscii / SetWindowsHookExA / SetWindowsHookExW)

#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <easyhook.h>
#include <iostream>
#include <string>
#include <random>
#include <vector>
#include <cstring>
#include <fstream>
#include <mutex>

#ifdef _WIN64
#pragma comment(lib, "EasyHook64.lib")
#else
#pragma comment(lib, "EasyHook32.lib")
#endif

using namespace std;

// Logging APIs call
std::mutex logMutex;
std::ofstream apiLog("api_log.txt", std::ios::app);

void LogApiCall(const std::string& apiName)
{
    std::lock_guard<std::mutex> lock(logMutex);
    if (apiLog.is_open())
    {
        apiLog << "[API Called] " << apiName << "()" << std::endl;
        apiLog.flush();
    }
}

// Random decoy char generator
char GenerateRandomChar()
{
    static std::string alphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dist(0, (int)alphabet.size() - 1);
    return alphabet[dist(gen)];
}

// Write decoy key to file
void LogDecoyKey(const std::string& source, char key)
{
    std::lock_guard<std::mutex> lock(logMutex);
	
	std::ofstream logFile("keylog.txt", std::ios::app);
    if (logFile.is_open())
    {
        logFile << " ======= DECOY - " << source << "] =======" << endl;
        logFile << key << endl;
        logFile.flush();
    }
}


// DLL Code of Clipboard logger

// Global Hook Handle
HOOK_TRACE_INFO hOpenClipboardHook = { NULL };
HOOK_TRACE_INFO hGetClipboardDataHook = { NULL };
HOOK_TRACE_INFO hSetClipboardDataHook = { NULL };

typedef BOOL(WINAPI* OpenClipboardFunc)(HWND);
typedef HANDLE(WINAPI* GetClipboardDataFunc)(UINT);
typedef HANDLE(WINAPI* SetClipboardDataFunc)(UINT, HANDLE);

OpenClipboardFunc     TrueOpenClipboard = nullptr;
GetClipboardDataFunc  TrueGetClipboardData = nullptr;
SetClipboardDataFunc  TrueSetClipboardData = nullptr;

// State Control - Enable deception only after OpenClipboard
bool deceptionMode = false;

// Hook Implementations

BOOL WINAPI myOpenClipboardHook(HWND hWndNewOwner)
{
    LogApiCall("OpenClipboard");
    std::cout << "[Hook] OpenClipboard intercepted.\n";
    deceptionMode = true;
    return TrueOpenClipboard(hWndNewOwner);
}

HANDLE WINAPI myGetClipboardDataHook(UINT uFormat)
{
    LogApiCall("GetClipboardData");
    std::cout << "[Hook] GetClipboardData intercepted. Format: " << uFormat << std::endl;

    if (deceptionMode)
    {
        std::cout << "[Deception] Returning decoy clipboard data.\n";

        // Universal decoy (always return CF_UNICODETEXT)
        const wchar_t* decoyW = L"DecoyPassword123!!!";
        size_t lenBytes = (wcslen(decoyW) + 1) * sizeof(wchar_t);

        // Log decoy to keylog.txt
        {
            std::lock_guard<std::mutex> lock(logMutex);
            std::wofstream wlog(L"keylog.txt", std::ios::app);
            if (wlog.is_open())
            {
                wlog << L"======= [DECOY - Clipboard CF_UNICODETEXT] =======\n";
                wlog << decoyW << L"\n\n";
                wlog.flush();
            }
        }
		
		// Allocate memory for clipboard
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, lenBytes);
        if (hMem)
        {
            void* ptr = GlobalLock(hMem);
            if (ptr)
            {
                memcpy(ptr, decoyW, lenBytes);
                GlobalUnlock(hMem);

                // Always return Unicode, regardless of request
                return hMem;
            }
        }

        // If allocation failed → still prevent real data leakage
        std::cout << "[Deception] Failed to allocate decoy. Returning empty string.\n";
        return nullptr;
    }

    // Normal behavior when deceptionMode is false
    return TrueGetClipboardData(uFormat);
}

HANDLE WINAPI mySetClipboardDataHook(UINT uFormat, HANDLE hMem)
{
    LogApiCall("SetClipboardData"); // [added logging]
    std::cout << "[Hook] SetClipboardData intercepted. Format: " << uFormat << std::endl;
    // (Optional) Add logic to log or modify outgoing clipboard data
    return TrueSetClipboardData(uFormat, hMem);
}


// DLL Code of Screen Logger

// Original function pointers
typedef BOOL(WINAPI* BitBltFunc)(
    HDC, int, int, int, int, HDC, int, int, DWORD);
BitBltFunc TrueBitBlt = nullptr;

typedef BOOL(WINAPI* PrintWindowFunc)(
    HWND, HDC, UINT);
PrintWindowFunc TruePrintWindow = nullptr;

// Decoy color
COLORREF decoyColor = RGB(255, 0, 0); // Red fill for obvious visual deception

// Hooked BitBlt
BOOL WINAPI myBitBltHook(
    HDC hdcDest, int nXDest, int nYDest,
    int nWidth, int nHeight,
    HDC hdcSrc, int nXSrc, int nYSrc,
    DWORD dwRop)
{
    LogApiCall("BitBlt");
    cout << "[Hook] BitBlt intercepted." << endl;

    // Fill the destination DC with decoy color
    HBRUSH brush = CreateSolidBrush(decoyColor);
    RECT rect = { nXDest, nYDest, nXDest + nWidth, nYDest + nHeight };
    FillRect(hdcDest, &rect, brush);
    DeleteObject(brush);

    // Pretend the operation succeeded
    return TRUE;
}

// Hooked PrintWindow
BOOL WINAPI myPrintWindowHook(
    HWND hwnd, HDC hdcBlt, UINT nFlags)
{
    LogApiCall("PrintWindow");
    cout << "[Hook] PrintWindow intercepted." << endl;

    // Get the window size
    RECT rc;
    GetWindowRect(hwnd, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;

    // Fill the window's DC with a decoy image (just solid fill here)
    HBRUSH brush = CreateSolidBrush(decoyColor);
    RECT fillRect = { 0, 0, width, height };
    FillRect(hdcBlt, &fillRect, brush);
    DeleteObject(brush);

    // Simulate success
    return TRUE;
}


// DLL Code of Mouse Tracking

// Global hook handle
HOOK_TRACE_INFO hCursorPosHook = { NULL };

// Fixed fake coordinates to mislead malware
const int FAKE_X = 9999;
const int FAKE_Y = 9999;

// Hooked version of GetCursorPos
BOOL WINAPI myGetCursorPosHook(LPPOINT lpPoint)
{
    LogApiCall("GetCursorPos"); // [added logging]

    lpPoint->x = FAKE_X;
    lpPoint->y = FAKE_Y;

    std::cout << "[Deception] GetCursorPos intercepted - returning fake position: X=" << FAKE_X << " Y=" << FAKE_Y << std::endl;
    return TRUE;
}


// DLL Code of Keyboard-Logging

// Hooked APIs
HOOK_TRACE_INFO hAsyncKeyHook = {};
HOOK_TRACE_INFO hKeyStateHook = {};
HOOK_TRACE_INFO hToAsciiHook = {};
HOOK_TRACE_INFO hHookInstall = {};

// Store the last decoy key to sync with ToAscii
thread_local char lastDecoyChar = 0;

// GetAsyncKeyState
typedef SHORT(WINAPI* GetAsyncKeyStateFunc)(int);
GetAsyncKeyStateFunc TrueGetAsyncKeyState = GetAsyncKeyState;

// Track per-VK press state across calls
bool pressedState[256] = { false };

SHORT WINAPI MyGetAsyncKeyStateHook(int vKey)
{
    SHORT actualState = TrueGetAsyncKeyState(vKey);

    bool isPressed = (actualState & 0x8000) != 0;

    if (isPressed && !pressedState[vKey])
    {
        // This is a new key press (was not pressed before)
        pressedState[vKey] = true;

        LogApiCall("GetAsyncKeyState");

        lastDecoyChar = GenerateRandomChar();
        LogDecoyKey("GetAsyncKeyState", lastDecoyChar);
        return 0x8000; // Simulate key press
    }

    if (!isPressed && pressedState[vKey])
    {
        // Key released — reset press state
        pressedState[vKey] = false;
    }

    return 0;
}

// GetKeyState
typedef SHORT(WINAPI* GetKeyStateFunc)(int);
GetKeyStateFunc TrueGetKeyState = GetKeyState;

SHORT WINAPI MyGetKeyStateHook(int vKey)
{
    LogApiCall("GetKeyState");

    if (vKey == VK_CAPITAL || vKey == VK_NUMLOCK || vKey == VK_SCROLL)
    {
        // Return accurate toggle state
        return TrueGetKeyState(vKey);
    }

    // For deception, simulate not pressed
    return 0;
}

// ToAscii
typedef int(WINAPI* ToAsciiFunc)(UINT, UINT, const BYTE*, LPWORD, UINT);
ToAsciiFunc TrueToAscii = ToAscii;

int WINAPI MyToAsciiHook(UINT vk, UINT scanCode, const BYTE* keyState, LPWORD lpChar, UINT flags)
{
    LogApiCall("ToAscii");

    if (lastDecoyChar != 0)
    {
        lpChar[0] = lastDecoyChar;
        LogDecoyKey("ToAscii", lastDecoyChar);
        return 1;
    }

    return TrueToAscii(vk, scanCode, keyState, lpChar, flags);
}

// SetWindowsHookEx
// SetWindowsHookExA and SetWindowsHookExW

typedef HHOOK(WINAPI* SetWindowsHookExAFunc)(int idHook, HOOKPROC lpfn, HINSTANCE hMod, DWORD dwThreadId);
typedef HHOOK(WINAPI* SetWindowsHookExWFunc)(int idHook, HOOKPROC lpfn, HINSTANCE hMod, DWORD dwThreadId);

SetWindowsHookExAFunc TrueSetWindowsHookExA = SetWindowsHookExA;
SetWindowsHookExWFunc TrueSetWindowsHookExW = SetWindowsHookExW;

HOOK_TRACE_INFO hSetHookExA = {};
HOOK_TRACE_INFO hSetHookExW = {};

// Shared dummy keyboard hook
LRESULT CALLBACK DummyKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0 && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN))
    {
        lastDecoyChar = GenerateRandomChar();
        LogDecoyKey("DummyKeyboardProc", lastDecoyChar);
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

// ANSI hook
HHOOK WINAPI MySetWindowsHookExAHook(int idHook, HOOKPROC lpfn, HINSTANCE hMod, DWORD dwThreadId)
{
    LogApiCall("SetWindowsHookExA");

    if (idHook == WH_KEYBOARD_LL)
    {
        std::cout << "[Deception] SetWindowsHookExA intercepted. Returning dummy hook.\n";

        // Return dummy handler instead of actual malware hook
        lastDecoyChar = GenerateRandomChar();
        LogDecoyKey("SetWindowsHookExA", lastDecoyChar);
        return TrueSetWindowsHookExA(idHook, DummyKeyboardProc, hMod, dwThreadId);
    }

    // For other hooks, proceed normally
    return TrueSetWindowsHookExA(idHook, lpfn, hMod, dwThreadId);
}

// Unicode hook
HHOOK WINAPI MySetWindowsHookExWHook(int idHook, HOOKPROC lpfn, HINSTANCE hMod, DWORD dwThreadId)
{
    LogApiCall("SetWindowsHookExW");

    if (idHook == WH_KEYBOARD_LL)
    {
        std::cout << "[Deception] SetWindowsHookExW intercepted. Returning dummy hook.\n";
        lastDecoyChar = GenerateRandomChar();
        LogDecoyKey("SetWindowsHookExW", lastDecoyChar);
        return TrueSetWindowsHookExW(idHook, DummyKeyboardProc, hMod, dwThreadId);
    }
    return TrueSetWindowsHookExW(idHook, lpfn, hMod, dwThreadId);
}


// Unified EasyHook entry
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
    // Install Clipboard hooks
    std::cout << "[*] DLL Injection Started.\n";

    HMODULE hUser32_forClipboard = GetModuleHandle(TEXT("user32"));
    if (!hUser32_forClipboard)
    {
        std::cerr << "[-] Failed to get handle to user32.dll\n";
    }
    else
    {
        TrueOpenClipboard = (OpenClipboardFunc)GetProcAddress(hUser32_forClipboard, "OpenClipboard");
        TrueGetClipboardData = (GetClipboardDataFunc)GetProcAddress(hUser32_forClipboard, "GetClipboardData");
        TrueSetClipboardData = (SetClipboardDataFunc)GetProcAddress(hUser32_forClipboard, "SetClipboardData");

        if (!TrueOpenClipboard || !TrueGetClipboardData || !TrueSetClipboardData)
        {
            std::cerr << "[-] Failed to resolve one or more clipboard APIs\n";
        }
        else
        {
            LhInstallHook(TrueOpenClipboard, myOpenClipboardHook, nullptr, &hOpenClipboardHook);
            LhInstallHook(TrueGetClipboardData, myGetClipboardDataHook, nullptr, &hGetClipboardDataHook);
            LhInstallHook(TrueSetClipboardData, mySetClipboardDataHook, nullptr, &hSetClipboardDataHook);

            // Enable for current process (malware process)
            ULONG ACLClip[1] = { 0 };
            LhSetExclusiveACL(ACLClip, 1, &hOpenClipboardHook);
            LhSetExclusiveACL(ACLClip, 1, &hGetClipboardDataHook);
            LhSetExclusiveACL(ACLClip, 1, &hSetClipboardDataHook);

            std::cout << "[+] Clipboard hooks installed.\n";
        }
    }

    // Install Screen logger hooks
    cout << "[*] Injection started (AntiScreenLoggerHook)." << endl;

    HOOK_TRACE_INFO hBitBltHook = { NULL };
    HOOK_TRACE_INFO hPrintWindowHook = { NULL };

    // Resolve API addresses
    HMODULE hGDI32 = GetModuleHandleW(L"gdi32.dll");
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");

    if (!hGDI32 || !hUser32)
    {
        cout << "[-] Failed to get module handles." << endl;
    }
    else
    {
        TrueBitBlt = (BitBltFunc)GetProcAddress(hGDI32, "BitBlt");
        TruePrintWindow = (PrintWindowFunc)GetProcAddress(hUser32, "PrintWindow");

        if (!TrueBitBlt || !TruePrintWindow)
        {
            cout << "[-] Failed to resolve original function addresses." << endl;
        }
        else
        {
            // Install hooks
            if (LhInstallHook(TrueBitBlt, myBitBltHook, nullptr, &hBitBltHook) != 0)
            {
                wcout << L"[-] Failed to hook BitBlt: " << RtlGetLastErrorString() << endl;
            }
            else
            {
                cout << "[+] BitBlt hook installed." << endl;
                ULONG acl[1] = { 0 };
                LhSetExclusiveACL(acl, 1, &hBitBltHook);
            }

            if (LhInstallHook(TruePrintWindow, myPrintWindowHook, nullptr, &hPrintWindowHook) != 0)
            {
                wcout << L"[-] Failed to hook PrintWindow: " << RtlGetLastErrorString() << endl;
            }
            else
            {
                cout << "[+] PrintWindow hook installed." << endl;
                ULONG acl[1] = { 0 };
                LhSetExclusiveACL(acl, 1, &hPrintWindowHook);
            }
        }
    }

    // Install Mouse tracking hook
    std::cout << "[*] Injection started (Mouse tracking deception)." << std::endl;

    FARPROC cursorPosAddr = GetProcAddress(GetModuleHandle(TEXT("user32")), "GetCursorPos");
    if (cursorPosAddr == nullptr)
    {
        std::cerr << "[!] Failed to get address of GetCursorPos." << std::endl;
    }
    else
    {
        NTSTATUS result = LhInstallHook(cursorPosAddr, myGetCursorPosHook, nullptr, &hCursorPosHook);
        if (FAILED(result))
        {
            std::wstring error = RtlGetLastErrorString();
            std::wcerr << L"[!] Failed to install GetCursorPos hook: " << error << std::endl;
        }
        else
        {
            ULONG ACLEntries[1] = { 0 };
            LhSetExclusiveACL(ACLEntries, 1, &hCursorPosHook);

            std::cout << "[+] GetCursorPos hook installed successfully!" << std::endl;
        }
    }

    // Install Keyboard deception hooks
    std::cout << "[*] Installing keyboard deception hooks..." << std::endl;

    // Resolve original addresses
    HMODULE user32 = GetModuleHandleA("user32.dll");

    FARPROC asyncAddr = GetProcAddress(user32, "GetAsyncKeyState");
    FARPROC keyStateAddr = GetProcAddress(user32, "GetKeyState");
    FARPROC toAsciiAddr = GetProcAddress(user32, "ToAscii");
    FARPROC setHookAAddr = GetProcAddress(GetModuleHandle(TEXT("user32")), "SetWindowsHookExA");
    FARPROC setHookWAddr = GetProcAddress(GetModuleHandle(TEXT("user32")), "SetWindowsHookExW");

    if (FAILED(LhInstallHook(asyncAddr, MyGetAsyncKeyStateHook, NULL, &hAsyncKeyHook)))
        std::cerr << "[!] Failed to hook GetAsyncKeyState." << std::endl;

    if (FAILED(LhInstallHook(keyStateAddr, MyGetKeyStateHook, NULL, &hKeyStateHook)))
        std::cerr << "[!] Failed to hook GetKeyState." << std::endl;

    if (FAILED(LhInstallHook(toAsciiAddr, MyToAsciiHook, NULL, &hToAsciiHook)))
        std::cerr << "[!] Failed to hook ToAscii." << std::endl;

    if (FAILED(LhInstallHook(setHookAAddr, MySetWindowsHookExAHook, NULL, &hSetHookExA)))
        std::cerr << "[!] Failed to hook SetWindowsHookExA" << std::endl;

    if (FAILED(LhInstallHook(setHookWAddr, MySetWindowsHookExWHook, NULL, &hSetHookExW)))
        std::cerr << "[!] Failed to hook SetWindowsHookExW" << std::endl;

    // Apply to all threads
    ULONG acl[1] = { 0 };
    LhSetExclusiveACL(acl, 1, &hAsyncKeyHook);
    LhSetExclusiveACL(acl, 1, &hKeyStateHook);
    LhSetExclusiveACL(acl, 1, &hToAsciiHook);
    LhSetExclusiveACL(acl, 1, &hSetHookExA);
    LhSetExclusiveACL(acl, 1, &hSetHookExW);

    std::cout << "[+] Deception hooks installed.\n";
}
