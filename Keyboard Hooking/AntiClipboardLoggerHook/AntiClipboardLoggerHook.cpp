#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <random>
#include <mutex>
#include <easyhook.h>

#pragma comment(lib, "EasyHook32.lib") // or EasyHook64.lib for x64 builds

using namespace std;

// ========================
// Globals
// ========================
std::ofstream logFile("keylog.txt", std::ios::app);
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
    if (logFile.is_open())
    {
        logFile << "======= [DECOY - " << source << "] =======" << endl;
        logFile << key << endl;
        logFile.flush();
    }
}

// ========================
// Hooked APIs
// ========================
HOOK_TRACE_INFO hAsyncKeyHook = {};
HOOK_TRACE_INFO hKeyStateHook = {};
HOOK_TRACE_INFO hToAsciiHook = {};
HOOK_TRACE_INFO hHookInstall = {};

// Store the last decoy key to sync with ToAscii
thread_local char lastDecoyChar = 0;

// ----------------- GetAsyncKeyState -----------------
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

// ----------------- GetKeyState -----------------
typedef SHORT(WINAPI* GetKeyStateFunc)(int);
GetKeyStateFunc TrueGetKeyState = GetKeyState;

SHORT WINAPI MyGetKeyStateHook(int vKey)
{
    // LogApiCall("GetKeyState");

    if (vKey == VK_CAPITAL || vKey == VK_NUMLOCK || vKey == VK_SCROLL)
    {
        // Return accurate toggle state
        return TrueGetKeyState(vKey);
    }

    // For deception, simulate not pressed
    return 0;
}

// ----------------- ToAscii -----------------
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

// ----------------- SetWindowsHookEx -----------------
// You could also optionally hook SetWindowsHookEx or intercept CallNextHookEx, but in this deception strategy,
// altering ToAscii is more effective for both Hook-based and Poll-based capture.

typedef HHOOK(WINAPI* SetWindowsHookExAFunc)(int idHook, HOOKPROC lpfn, HINSTANCE hMod, DWORD dwThreadId);
SetWindowsHookExAFunc TrueSetWindowsHookExA = SetWindowsHookExA;

LRESULT CALLBACK DummyKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    // Simulate benign behavior
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

HOOK_TRACE_INFO hSetHookEx = {};

HHOOK WINAPI MySetWindowsHookExAHook(int idHook, HOOKPROC lpfn, HINSTANCE hMod, DWORD dwThreadId)
{
    LogApiCall("SetWindowsHookExA");

    if (idHook == WH_KEYBOARD_LL)
    {
        std::cout << "[Deception] SetWindowsHookExA intercepted. Returning dummy hook.\n";
        // Return dummy handler instead of actual malware hook
        return TrueSetWindowsHookExA(idHook, DummyKeyboardProc, hMod, dwThreadId);
    }

    // For other hooks, proceed normally
    return TrueSetWindowsHookExA(idHook, lpfn, hMod, dwThreadId);
}


// ========================
// DLL Entry Hook
// ========================
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
    std::cout << "[*] Anti-keylogger deception DLL injected." << std::endl;

    // Resolve original addresses
    HMODULE user32 = GetModuleHandleA("user32.dll");

    FARPROC asyncAddr = GetProcAddress(user32, "GetAsyncKeyState");
    FARPROC keyStateAddr = GetProcAddress(user32, "GetKeyState");
    FARPROC toAsciiAddr = GetProcAddress(user32, "ToAscii");
    FARPROC setHookAddr = GetProcAddress(GetModuleHandle(TEXT("user32")), "SetWindowsHookExA");

    if (FAILED(LhInstallHook(asyncAddr, MyGetAsyncKeyStateHook, NULL, &hAsyncKeyHook)))
        std::cerr << "[!] Failed to hook GetAsyncKeyState." << std::endl;

    if (FAILED(LhInstallHook(keyStateAddr, MyGetKeyStateHook, NULL, &hKeyStateHook)))
        std::cerr << "[!] Failed to hook GetKeyState." << std::endl;

    if (FAILED(LhInstallHook(toAsciiAddr, MyToAsciiHook, NULL, &hToAsciiHook)))
        std::cerr << "[!] Failed to hook ToAscii." << std::endl;

    if (FAILED(LhInstallHook(setHookAddr, MySetWindowsHookExAHook, NULL, &hSetHookEx)))
        std:cerr << "[!] Failed to hook SetWindowsHookExA" << std::endl;

    // Apply to all threads (including this process)
    ULONG acl[1] = { 0 };
    LhSetExclusiveACL(acl, 1, &hAsyncKeyHook);
    LhSetExclusiveACL(acl, 1, &hKeyStateHook);
    LhSetExclusiveACL(acl, 1, &hToAsciiHook);
    LhSetExclusiveACL(acl, 1, &hSetHookEx);

    std::cout << "[+] Deception hooks installed.\n";
}
