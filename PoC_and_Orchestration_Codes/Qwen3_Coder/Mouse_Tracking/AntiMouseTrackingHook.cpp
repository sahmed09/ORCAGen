#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <easyhook.h>
#include <thread>
#include <chrono>

using namespace std;

// Fake coordinate values for deception
const int FAKE_X = 100;
const int FAKE_Y = 100;

// Store the original function pointer
typedef BOOL(WINAPI* GetCursorPosFunc)(LPPOINT lpPoint);
GetCursorPosFunc OriginalGetCursorPos = nullptr;

// Hooked version of GetCursorPos
BOOL WINAPI MyGetCursorPos(LPPOINT lpPoint)
{
    // Get current process ID and thread ID
    DWORD currentProcessId = GetCurrentProcessId();
    DWORD currentThreadId = GetCurrentThreadId();

    // Log for debugging or detection (optional)
    cout << "[Defense] GetCursorPos called from PID: " << currentProcessId
        << ", TID: " << currentThreadId << endl;

    // Detect if this is likely the malicious process (e.g., based on name)
    char processName[MAX_PATH] = { 0 };
    GetModuleFileNameA(NULL, processName, MAX_PATH);
    string exeName = processName;
    size_t pos = exeName.find_last_of("\\/");
    if (pos != string::npos)
        exeName = exeName.substr(pos + 1);

    if (exeName.find("ClipboardLogger") != string::npos ||
        exeName.find("malware") != string::npos ||
        exeName.find("logger") != string::npos)
    {
        cout << "[BLOCKED] Mouse tracking attempt blocked by defense mechanism." << endl;
        if (lpPoint != nullptr)
        {
            lpPoint->x = FAKE_X;
            lpPoint->y = FAKE_Y;
        }
        return TRUE; // Still return success for compatibility
    }

    // Normal operation - delegate to original function
    return OriginalGetCursorPos(lpPoint);
}


// Injection entry point
extern "C" __declspec(dllexport) void __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
    cout << "[*] Mouse tracking defense hook initialized." << endl;

    HOOK_TRACE_INFO hHook = { NULL };

    // Get address of original function
    FARPROC targetFunc = GetProcAddress(GetModuleHandle(TEXT("user32")), "GetCursorPos");
    if (targetFunc == nullptr)
    {
        cout << "[ERROR] Failed to locate GetCursorPos." << endl;
        return;
    }

    OriginalGetCursorPos = (GetCursorPosFunc)targetFunc;

    // Install hook
    if (FAILED(LhInstallHook(targetFunc, MyGetCursorPos, nullptr, &hHook)))
    {
        cout << "[ERROR] Failed to install hook on GetCursorPos." << endl;
        return;
    }

    // Enable the hook
    ULONG ACLEntries[1] = { 0 };
    if (FAILED(LhSetExclusiveACL(ACLEntries, 1, &hHook)))
    {
        cout << "[ERROR] Failed to set ACL for hook." << endl;
        return;
    }

    cout << "[SUCCESS] GetCursorPos hooked successfully." << endl;
}