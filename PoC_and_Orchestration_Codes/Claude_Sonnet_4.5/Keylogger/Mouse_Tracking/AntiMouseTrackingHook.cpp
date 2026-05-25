#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <easyhook.h>
#include <random>
#include <chrono>

#pragma comment(lib, "EasyHook32.lib")  // or EasyHook64.lib depending on platform

using namespace std;

// Random number generator for deception
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> offsetDist(-50, 50);  // Random offset range
std::uniform_int_distribution<> chanceDist(0, 99);

// Track legitimate vs suspicious call patterns
struct CallTracker {
    int callCount = 0;
    DWORD lastCallTime = 0;
    bool isSuspicious = false;
};

CallTracker g_tracker;

// Original GetCursorPos function pointer
typedef BOOL(WINAPI* GetCursorPos_t)(LPPOINT);
GetCursorPos_t OriginalGetCursorPos = nullptr;

// Heuristic to detect suspicious mouse tracking behavior
bool IsSuspiciousTracking()
{
    DWORD currentTime = GetTickCount();
    DWORD timeDiff = currentTime - g_tracker.lastCallTime;

    g_tracker.callCount++;
    g_tracker.lastCallTime = currentTime;

    // Heuristic: Rapid polling (< 150ms between calls) is suspicious
    if (timeDiff < 150 && g_tracker.callCount > 5)
    {
        g_tracker.isSuspicious = true;
        return true;
    }

    // Reset counter after reasonable time gap
    if (timeDiff > 1000)
    {
        g_tracker.callCount = 0;
        g_tracker.isSuspicious = false;
    }

    return g_tracker.isSuspicious;
}

// Hooked GetCursorPos function
BOOL WINAPI myGetCursorPosHook(LPPOINT lpPoint)
{
    // Call original function to get real position
    BOOL result = GetCursorPos(lpPoint);

    if (!result || lpPoint == nullptr)
    {
        return result;
    }

    // Check if this looks like malicious tracking
    if (IsSuspiciousTracking())
    {
        // Strategy 1: Return fake coordinates (deception)
        POINT fakePos;
        fakePos.x = lpPoint->x + offsetDist(gen);
        fakePos.y = lpPoint->y + offsetDist(gen);

        // Ensure fake coordinates stay within reasonable screen bounds
        fakePos.x = max(0, min(fakePos.x, 1920));
        fakePos.y = max(0, min(fakePos.y, 1080));

        cout << "[DECEPTION] GetCursorPos hooked - Suspicious tracking detected!" << endl;
        cout << "  Real Position: (" << lpPoint->x << ", " << lpPoint->y << ")" << endl;
        cout << "  Fake Position: (" << fakePos.x << ", " << fakePos.y << ")" << endl;

        // Return fake position to deceive malware
        lpPoint->x = fakePos.x;
        lpPoint->y = fakePos.y;

        // Alternative Strategy 2: Block with error (uncomment to use instead)
        /*
        cout << "[BLOCK] GetCursorPos blocked - Suspicious tracking detected!" << endl;
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
        */
    }
    else
    {
        // Legitimate use - allow real coordinates to pass through
        cout << "[ALLOW] GetCursorPos - Legitimate call: ("
            << lpPoint->x << ", " << lpPoint->y << ")" << endl;
    }

    return result;
}

// Hooked Beep function (from original codebase)
DWORD gFreqOffset = 0;

BOOL WINAPI myBeepHook(DWORD dwFreq, DWORD dwDuration)
{
    cout << "[+] BeepHook triggered!" << endl;
    cout << "Original Frequency: " << dwFreq << ", Duration: " << dwDuration << endl;
    return Beep(dwFreq + gFreqOffset, dwDuration);
}

// DLL Entry Point for EasyHook
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
    cout << "[*] AntiMouseTrackingHook injection started." << endl;

    // Process user data if provided
    if (inRemoteInfo->UserDataSize == sizeof(DWORD))
    {
        gFreqOffset = *reinterpret_cast<DWORD*>(inRemoteInfo->UserData);
        cout << "Frequency Offset Received: " << gFreqOffset << endl;
    }

    // Hook structures
    HOOK_TRACE_INFO hBeepHook = { NULL };
    HOOK_TRACE_INFO hGetCursorPosHook = { NULL };

    // Install Beep hook (original functionality)
    FARPROC beepAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "Beep");
    if (beepAddr)
    {
        NTSTATUS result = LhInstallHook(beepAddr, myBeepHook, nullptr, &hBeepHook);
        if (FAILED(result))
        {
            wstring s(RtlGetLastErrorString());
            wcerr << L"Failed to install Beep hook: " << s << endl;
        }
        else
        {
            cout << "[+] Beep hook installed successfully!" << endl;
        }
    }

    // Install GetCursorPos hook (new functionality)
    FARPROC getCursorPosAddr = GetProcAddress(GetModuleHandle(TEXT("user32")), "GetCursorPos");
    if (getCursorPosAddr)
    {
        OriginalGetCursorPos = (GetCursorPos_t)getCursorPosAddr;

        NTSTATUS result = LhInstallHook(getCursorPosAddr, myGetCursorPosHook, nullptr, &hGetCursorPosHook);
        if (FAILED(result))
        {
            wstring s(RtlGetLastErrorString());
            wcerr << L"Failed to install GetCursorPos hook: " << s << endl;
        }
        else
        {
            cout << "[+] GetCursorPos hook installed successfully!" << endl;
            cout << "[*] Mouse tracking deception active!" << endl;
        }
    }

    // Enable hooks for all threads
    ULONG ACLEntries[1] = { 0 };
    LhSetExclusiveACL(ACLEntries, 1, &hBeepHook);
    LhSetExclusiveACL(ACLEntries, 1, &hGetCursorPosHook);

    cout << "[*] All hooks activated." << endl;
}