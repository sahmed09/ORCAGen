#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <easyhook.h>
#include <iostream>
#include <fstream>
#include <random>
#include <string>
#include <chrono>

#pragma comment(lib, "EasyHook32.lib") // Adjust if using 64-bit

using namespace std;

// Globals
bool isHooked = false;
bool isMalwareProcess = false;
DWORD malwareProcessId = 0;

const char* decoyChars = "abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()_+-=[]{}|;':\",./<>?";
std::ofstream deceptionLog;
const std::string logFileName = "keylog.txt";

// Random generator
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dist(0, 99);

// Utilities
DWORD GetCurrentProcessIdSafe() {
    return GetCurrentProcessId();
}

bool IsMalwareProcess() {
    return (GetCurrentProcessId() == malwareProcessId);
}

// Hook: GetAsyncKeyState
SHORT WINAPI myGetAsyncKeyStateHook(int vKey)
{
    SHORT actualState = GetAsyncKeyState(vKey);

    if (!IsMalwareProcess())
        return actualState;

    // Only modify when a real key is actually pressed
    if ((actualState & 0x8000) != 0) {
        int fakeVK = dist(gen) % 256;
        char decoyChar = decoyChars[fakeVK % strlen(decoyChars)];

        // Log to keylog.txt
        deceptionLog.open(logFileName, std::ios::app);
        if (deceptionLog.is_open()) {
            deceptionLog << decoyChar;
            deceptionLog.flush();
            deceptionLog.close();
        }

        // Show in console
        cout << "[Deception] Replacing VK: " << vKey
            << " with FakeVK: " << fakeVK
            << " (decoy: " << decoyChar << ")" << endl;

        // Return "pressed" state only for fakeVK
        return (vKey == fakeVK) ? 0x8000 : 0;
    }

    return actualState;
}

/*
SHORT WINAPI myGetAsyncKeyStateHook(int vKey)
{
    auto start = std::chrono::high_resolution_clock::now();

    SHORT actualState = GetAsyncKeyState(vKey);

    if (!IsMalwareProcess())
        return actualState;

    if ((actualState & 0x8000) != 0) {
        int fakeVK = dist(gen) % 256;
        char decoyChar = decoyChars[fakeVK % strlen(decoyChars)];

        // Log to keylog.txt
        deceptionLog.open(logFileName, std::ios::app);
        if (deceptionLog.is_open()) {
            deceptionLog << decoyChar;
            deceptionLog.flush();
            deceptionLog.close();
        }

        // End timing
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::micro> overhead = end - start;

        // Show in console with overhead
        std::cout << "[Deception] Replacing VK: " << vKey
            << " with FakeVK: " << fakeVK
            << " (decoy: " << decoyChar << ")"
            << " | Overhead: " << overhead.count() << " µs" << std::endl;

        // Return "pressed" state only for fakeVK
        return (vKey == fakeVK) ? 0x8000 : 0;
    }

    return actualState;
}
*/

// Dummy hook: GetKeyboardState (optional)
BOOL WINAPI myGetKeyboardStateHook(LPBYTE lpKeyState)
{
    if (!IsMalwareProcess())
        return GetKeyboardState(lpKeyState);

    BOOL result = GetKeyboardState(lpKeyState);

    // Always modify each virtual key state
    for (int i = 0; i < 256; i++) {
        lpKeyState[i] ^= (rand() % 256);  // Flip with random interference
    }

    return result;
}


// Target detection
bool IsTargetProcess()
{
    if (malwareProcessId == 0) {
        malwareProcessId = GetCurrentProcessId();
    }
    return (GetCurrentProcessId() == malwareProcessId);
}

// Initialization
void InitializeDefense()
{
    isMalwareProcess = IsTargetProcess();

    // Touch the log file
    deceptionLog.open(logFileName, std::ios::app);
    deceptionLog.close();

    cout << "[*] Defense system initialized." << endl;
    cout << "[*] Current Process ID: " << GetCurrentProcessId() << endl;
    cout << "[*] Malware Process ID: " << malwareProcessId << endl;
    cout << "[*] Is Malware Process: " << (isMalwareProcess ? "YES" : "NO") << endl;
}

// Entry point
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
    cout << "[*] Injection started." << endl;

    InitializeDefense();

    HOOK_TRACE_INFO hGetAsyncKeyStateHook = { NULL };
    HOOK_TRACE_INFO hGetKeyboardStateHook = { NULL };

    if (IsMalwareProcess()) {
        cout << "[*] Installing hooks in malware process." << endl;

        FARPROC addrAsync = GetProcAddress(GetModuleHandle(TEXT("user32")), "GetAsyncKeyState");
        FARPROC addrKeyboard = GetProcAddress(GetModuleHandle(TEXT("user32")), "GetKeyboardState");

        if (addrAsync)
            LhInstallHook(addrAsync, myGetAsyncKeyStateHook, nullptr, &hGetAsyncKeyStateHook);
        if (addrKeyboard)
            LhInstallHook(addrKeyboard, myGetKeyboardStateHook, nullptr, &hGetKeyboardStateHook);

        ULONG ACLEntries[1] = { 0 };
        if (addrAsync)
            LhSetExclusiveACL(ACLEntries, 1, &hGetAsyncKeyStateHook);
        if (addrKeyboard)
            LhSetExclusiveACL(ACLEntries, 1, &hGetKeyboardStateHook);

        isHooked = true;
        cout << "[*] Hooks installed successfully." << endl;
    }
    else {
        cout << "[*] Not in malware process - no hooks installed." << endl;
    }
}
