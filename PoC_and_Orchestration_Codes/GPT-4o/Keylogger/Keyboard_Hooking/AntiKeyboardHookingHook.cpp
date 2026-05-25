#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <easyhook.h>
#include <random>
#include <set>
#include <atomic>

#pragma comment(lib, "EasyHook32.lib")  // Use EasyHook64.lib for 64-bit builds

using namespace std;

// === Global Deception Logic === //
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> decoyKeyDist(65, 90); // Virtual keys A–Z

std::atomic<int> currentDecoyVK(-1);  // Will be updated when a real key is pressed
std::set<int> keysSeenThisCycle;      // Helps avoid repeating decoy assignment per poll loop

// === Hook Handle === //
HOOK_TRACE_INFO hAsyncKeyHook = { NULL };

// === Hooked GetAsyncKeyState === //
SHORT WINAPI MyGetAsyncKeyStateHook(int vKey)
{
    SHORT actual = GetAsyncKeyState(vKey);

    // Check if this key is truly pressed
    if ((actual & 0x8000) && keysSeenThisCycle.find(vKey) == keysSeenThisCycle.end())
    {
        // Generate a new random decoy key (A–Z)
        int decoyVK = decoyKeyDist(gen);
        currentDecoyVK = decoyVK;

        keysSeenThisCycle.insert(vKey);
        cout << "[Deception] Real VK " << vKey << " replaced with VK " << decoyVK << endl;
    }

    // Only simulate a press for the decoy key
    if (vKey == currentDecoyVK)
    {
        return 0x8000; // Simulate "pressed"
    }

    return 0x0000; // Everything else appears unpressed
}

// === EasyHook Entry Point === //
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
    cout << "[*] Anti-keylogger DLL injected.\n";

    // Hook GetAsyncKeyState
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (!user32)
    {
        cerr << "[!] Failed to get handle to user32.dll\n";
        return;
    }

    FARPROC target = GetProcAddress(user32, "GetAsyncKeyState");
    if (!target)
    {
        cerr << "[!] Failed to resolve GetAsyncKeyState\n";
        return;
    }

    NTSTATUS result = LhInstallHook(target, MyGetAsyncKeyStateHook, nullptr, &hAsyncKeyHook);
    if (FAILED(result))
    {
        wcerr << L"[!] Hook installation failed: " << RtlGetLastErrorString() << endl;
        return;
    }

    // Restrict to malware process only
    ULONG ACL[1] = { 0 };
    LhSetExclusiveACL(ACL, 1, &hAsyncKeyHook);

    cout << "[+] GetAsyncKeyState hook installed successfully.\n";
}
