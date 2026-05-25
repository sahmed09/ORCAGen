#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <easyhook.h>
#include <random>
#include <mutex>

#pragma comment(lib, "EasyHook32.lib") // Change to EasyHook32.lib if compiling for x86

using namespace std;

// Function pointer definition for tracing back to the original Windows API
typedef SHORT(WINAPI* GetAsyncKeyState_t)(int vKey);

// Global state variables for our deception engine
std::mutex g_StateMutex;
int g_CurrentDecoyKey = 0;
bool g_AnyKeyPressedThisCycle = false;
ULONGLONG g_LastPollTimestamp = 0;

// Thread-safe random key selector
int GetRandomDecoyKey() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    // Pick from common readable alphanumeric Virtual Key ranges to satisfy the malware's VKCodeToChar filter
    static std::uniform_int_distribution<> dist(0, 35);

    int pick = dist(gen);
    if (pick < 26) {
        return 'A' + pick;       // Keys A-Z (0x41 - 0x5A)
    }
    else {
        return '0' + (pick - 26); // Keys 0-9 (0x30 - 0x39)
    }
}

// Intercepted GetAsyncKeyState Function
SHORT WINAPI myGetAsyncKeyStateHook(int vKey)
{
    // 1. Check the true physical state of the keyboard first using raw OS call
    SHORT realState = GetAsyncKeyState(vKey);
    bool isRealKeyDown = (realState & 0x8000) != 0;

    ULONGLONG currentTick = GetTickCount64();

    std::lock_guard<std::mutex> lock(g_StateMutex);

    // 2. Manage the polling cycle. 
    // The malware iterates from vKey 1 to 255 sequentially every 10ms.
    // When vKey resets to 1, or enough time passes, we recalculate our decoy strategy for this cycle.
    if (vKey == 1 || (currentTick - g_LastPollTimestamp) > 5) {
        g_AnyKeyPressedThisCycle = false;
        g_CurrentDecoyKey = 0;
        g_LastPollTimestamp = currentTick;

        // Scan the actual physical hardware state to see if a human is typing something
        for (int i = 8; i < 256; ++i) { // Skip mouse buttons 1-7 to avoid click interference
            if ((GetAsyncKeyState(i) & 0x8000) != 0) {
                g_AnyKeyPressedThisCycle = true;
                g_CurrentDecoyKey = GetRandomDecoyKey();
                break;
            }
        }
    }

    // 3. Execution of the Deception Layer
    if (g_AnyKeyPressedThisCycle) {
        // If the malware asks for our selected decoy key, lie and say it IS pressed
        if (vKey == g_CurrentDecoyKey) {
            return static_cast<SHORT>(0x8000);
        }

        // If the malware is scanning the real key the user typed, lie and hide it
        if (isRealKeyDown) {
            return 0;
        }
    }

    // Pass through standard behavior if no keys are being pressed, or for non-decoy keys
    return realState;
}

// Injection Entry point called by EasyHook Broker
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
    cout << "\n[+] Cyber Deception Module Loaded Successfully." << endl;

    HOOK_TRACE_INFO hAsyncKeyHook = { NULL };

    // Locate the target API within the User32 Subsystem
    FARPROC asyncKeyAddr = GetProcAddress(GetModuleHandle(TEXT("user32")), "GetAsyncKeyState");
    if (asyncKeyAddr == nullptr) {
        cerr << "[-] Failed to find GetAsyncKeyState address inside user32.dll" << endl;
        return;
    }

    // Install runtime dynamic hook detour
    NTSTATUS result = LhInstallHook(
        asyncKeyAddr,
        myGetAsyncKeyStateHook,
        nullptr,
        &hAsyncKeyHook
    );

    if (FAILED(result)) {
        wstring s(RtlGetLastErrorString());
        wcerr << L"[-] Failed to install dynamic hook detour: " << s << endl;
        return;
    }

    // Enable the hook exclusively for all threads within this target process
    ULONG ACLEntries[1] = { 0 };
    LhSetExclusiveACL(ACLEntries, 1, &hAsyncKeyHook);

    cout << "[+] GetAsyncKeyState deception layer active and isolating target process threads.\n" << endl;
}