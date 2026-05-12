#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <easyhook.h>
#include <random>
#include <vector>
#include <ctime>

#pragma comment(lib, "EasyHook32.lib")  // or EasyHook64.lib

using namespace std;

// Global variables for deception
std::vector<std::string> decoyClipboardData = {
    "john.doe@example.com - Password: DemoPass123!",
    "Credit Card: 4532-1111-2222-3333 CVV: 123 Exp: 12/25",
    "SSH Key: ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAABAQ...",
    "API Token: sk_test_4eC39HqLyjWDarjtT1zdp7dc",
    "Username: admin Password: CompanySecret2024",
    "Bank Account: 9876543210 Routing: 123456789"
};

std::random_device rd;
std::mt19937 gen(rd());

// Logging function for detection/monitoring
void LogSuspiciousActivity(const std::string& message)
{
    std::ofstream logFile("hook_detection_log.txt", std::ios::app);
    if (logFile.is_open())
    {
        time_t now = time(0);
        char buf[80];
        struct tm timeinfo;
        localtime_s(&timeinfo, &now);
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);

        logFile << "[" << std::string(buf) << "] " << message << "\n";
        logFile.close();
    }
}

// Original function pointers
typedef HANDLE(WINAPI* GetClipboardData_t)(UINT uFormat);
typedef BOOL(WINAPI* OpenClipboard_t)(HWND hWndNewOwner);

GetClipboardData_t OriginalGetClipboardData = nullptr;
OpenClipboard_t OriginalOpenClipboard = nullptr;

// Global handle for our decoy data
HGLOBAL g_hDecoyData = nullptr;

// Hook: OpenClipboard - Monitor clipboard access
BOOL WINAPI myOpenClipboardHook(HWND hWndNewOwner)
{
    cout << "[HOOK] OpenClipboard intercepted!" << endl;

    // Log suspicious activity
    DWORD pid = GetCurrentProcessId();
    char logMsg[256];
    sprintf_s(logMsg, "Clipboard access detected by PID: %lu", pid);
    LogSuspiciousActivity(logMsg);

    // Call original function to maintain normal behavior
    return OriginalOpenClipboard(hWndNewOwner);
}

// Hook: GetClipboardData - Replace with decoy content
HANDLE WINAPI myGetClipboardDataHook(UINT uFormat)
{
    cout << "[HOOK] GetClipboardData intercepted! Format: " << uFormat << endl;

    // Only intercept text format (CF_TEXT)
    if (uFormat == CF_TEXT)
    {
        cout << "[DECEPTION] Injecting decoy clipboard data..." << endl;
        LogSuspiciousActivity("Clipboard data access intercepted - Decoy data injected");

        // Select random decoy data
        std::uniform_int_distribution<size_t> dist(0, decoyClipboardData.size() - 1);
        size_t index = dist(gen);
        std::string decoyText = decoyClipboardData[index];

        cout << "[DECEPTION] Serving fake data: " << decoyText.substr(0, 30) << "..." << endl;

        // Free previous decoy data if it exists
        if (g_hDecoyData != nullptr)
        {
            GlobalFree(g_hDecoyData);
        }

        // Allocate global memory for decoy data
        size_t dataSize = decoyText.length() + 1;
        g_hDecoyData = GlobalAlloc(GMEM_MOVEABLE, dataSize);

        if (g_hDecoyData != nullptr)
        {
            char* pDecoyData = static_cast<char*>(GlobalLock(g_hDecoyData));
            if (pDecoyData != nullptr)
            {
                strcpy_s(pDecoyData, dataSize, decoyText.c_str());
                GlobalUnlock(g_hDecoyData);

                // Return our decoy data instead of real clipboard
                return g_hDecoyData;
            }
        }
    }

    // For non-text formats or if allocation failed, call original
    return OriginalGetClipboardData(uFormat);
}

// Hook: Beep (for compatibility with existing test framework)
BOOL WINAPI myBeepHook(DWORD dwFreq, DWORD dwDuration)
{
    cout << "[HOOK] Beep intercepted! Freq: " << dwFreq << ", Duration: " << dwDuration << endl;
    return Beep(dwFreq, dwDuration);
}

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
    cout << "========================================" << endl;
    cout << "[*] Anti-Clipboard Logger Hook Loaded" << endl;
    cout << "[*] Defensive Deception Active" << endl;
    cout << "========================================" << endl;

    LogSuspiciousActivity("Hook DLL injected - Monitoring started");

    HOOK_TRACE_INFO hBeepHook = { NULL };
    HOOK_TRACE_INFO hOpenClipboardHook = { NULL };
    HOOK_TRACE_INFO hGetClipboardDataHook = { NULL };

    // Get function addresses
    FARPROC beepAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "Beep");
    FARPROC openClipboardAddr = GetProcAddress(GetModuleHandle(TEXT("user32")), "OpenClipboard");
    FARPROC getClipboardDataAddr = GetProcAddress(GetModuleHandle(TEXT("user32")), "GetClipboardData");

    // Store original function pointers
    OriginalOpenClipboard = reinterpret_cast<OpenClipboard_t>(openClipboardAddr);
    OriginalGetClipboardData = reinterpret_cast<GetClipboardData_t>(getClipboardDataAddr);

    // Install hooks
    NTSTATUS result;

    // Hook Beep (for testing)
    result = LhInstallHook(beepAddr, myBeepHook, nullptr, &hBeepHook);
    if (FAILED(result))
    {
        wstring s(RtlGetLastErrorString());
        wcerr << L"[ERROR] Failed to hook Beep: " << s << endl;
    }
    else
    {
        cout << "[SUCCESS] Beep hook installed" << endl;
    }

    // Hook OpenClipboard (for monitoring)
    result = LhInstallHook(openClipboardAddr, myOpenClipboardHook, nullptr, &hOpenClipboardHook);
    if (FAILED(result))
    {
        wstring s(RtlGetLastErrorString());
        wcerr << L"[ERROR] Failed to hook OpenClipboard: " << s << endl;
    }
    else
    {
        cout << "[SUCCESS] OpenClipboard hook installed" << endl;
    }

    // Hook GetClipboardData (for deception)
    result = LhInstallHook(getClipboardDataAddr, myGetClipboardDataHook, nullptr, &hGetClipboardDataHook);
    if (FAILED(result))
    {
        wstring s(RtlGetLastErrorString());
        wcerr << L"[ERROR] Failed to hook GetClipboardData: " << s << endl;
    }
    else
    {
        cout << "[SUCCESS] GetClipboardData hook installed" << endl;
    }

    // Enable hooks for all threads (ACL with thread ID 0)
    ULONG ACLEntries[1] = { 0 };
    LhSetExclusiveACL(ACLEntries, 1, &hBeepHook);
    LhSetExclusiveACL(ACLEntries, 1, &hOpenClipboardHook);
    LhSetExclusiveACL(ACLEntries, 1, &hGetClipboardDataHook);

    cout << "[*] All hooks activated successfully!" << endl;
    cout << "[*] Clipboard deception layer is now active" << endl;
    LogSuspiciousActivity("All hooks activated successfully");
}