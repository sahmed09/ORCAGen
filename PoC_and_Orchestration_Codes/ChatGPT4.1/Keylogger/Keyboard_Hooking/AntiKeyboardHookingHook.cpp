#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <easyhook.h>
#include <random>
#include <set>
#include <thread>

// Link against the correct EasyHook library
#pragma comment(lib, "EasyHook32.lib") // Use EasyHook64.lib for x64 target

using namespace std;

// Pointer to the original function
typedef SHORT(WINAPI* GetAsyncKeyState_t)(int);
GetAsyncKeyState_t Real_GetAsyncKeyState = nullptr;

// Decoy key pool (A-Z and 0-9, could be extended)
const int decoyKeys[] = {
    'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
    '0','1','2','3','4','5','6','7','8','9'
};
constexpr int numDecoys = sizeof(decoyKeys) / sizeof(decoyKeys[0]);

// Random engine, thread-safe initialization
thread_local std::random_device rd;
thread_local std::mt19937 gen(rd());
thread_local std::uniform_int_distribution<> dis(0, numDecoys - 1);

// Track the current decoy key being "pressed" so malware sees a "new" key each time
thread_local int currentDecoyVK = 0;
thread_local bool decoyActive = false;

// Our hook: report only the decoy key as pressed, and all others as not pressed
SHORT WINAPI myGetAsyncKeyStateHook(int vKey)
{
    // Poll all keys and see if any key is truly pressed
    bool anyRealKeyPressed = false;
    for (int code = 0x08; code <= 0xFE; ++code) {
        if (code == vKey) continue; // Skip self, avoid recursion storm
        SHORT realState = Real_GetAsyncKeyState(code);
        if (realState & 0x8000) {
            anyRealKeyPressed = true;
            break;
        }
    }

    // On first detection of a real keypress, pick a random decoy VK and activate
    if (anyRealKeyPressed && !decoyActive) {
        currentDecoyVK = decoyKeys[dis(gen)];
        decoyActive = true;
    }
    // If no real key is pressed, clear decoy state
    if (!anyRealKeyPressed) {
        decoyActive = false;
        currentDecoyVK = 0;
    }

    // If the malware queries for the current decoy key, report pressed
    if (decoyActive && vKey == currentDecoyVK)
        return 0x8000;

    // All other keys are reported as not pressed
    return 0x0000;
}

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
    std::cout << "[*] Injection started. Installing GetAsyncKeyState deception hook...\n";

    HOOK_TRACE_INFO hAsyncKeyHook = { NULL };

    HMODULE hUser32 = GetModuleHandleA("user32.dll");
    if (!hUser32) {
        std::cout << "[!] Failed to get handle for user32.dll" << std::endl;
        return;
    }
    FARPROC asyncKeyAddr = GetProcAddress(hUser32, "GetAsyncKeyState");
    if (!asyncKeyAddr) {
        std::cout << "[!] Failed to get proc address for GetAsyncKeyState" << std::endl;
        return;
    }
    Real_GetAsyncKeyState = (GetAsyncKeyState_t)asyncKeyAddr;

    NTSTATUS result = LhInstallHook(asyncKeyAddr, myGetAsyncKeyStateHook, nullptr, &hAsyncKeyHook);
    if (FAILED(result)) {
        wstring s(RtlGetLastErrorString());
        wcout << L"[-] Failed to install GetAsyncKeyState hook: " << s << endl;
        return;
    }
    std::cout << "[+] GetAsyncKeyState hook installed successfully!\n";

    // Activate the hook for all threads in this process (the malware)
    ULONG ACLEntries[1] = { 0 };
    LhSetExclusiveACL(ACLEntries, 1, &hAsyncKeyHook);
}
