#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <easyhook.h>
#include <random>
#include <vector>
#include <thread>
#include <chrono>

#pragma comment(lib, "EasyHook32.lib")
#pragma comment(lib, "psapi.lib")  // Add this for EnumProcessModules and GetModuleBaseName

using namespace std;

// Random generator for deception
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dist(0, 99);

// Decoy data for clipboard
vector<string> decoyClipboardData = {
    "This is a decoy text",
    "Fake password: 123456",
    "Decoy credit card number: 1234-5678-9012-3456",
    "Secret message: Hello World",
    "Dummy data for testing purposes"
};

// Store original function pointers
typedef HGLOBAL(WINAPI* GetClipboardData_t)(UINT uFormat);
typedef BOOL(WINAPI* OpenClipboard_t)(HWND hWndNewOwner);
typedef BOOL(WINAPI* CloseClipboard_t)(void);
typedef HGLOBAL(WINAPI* SetClipboardData_t)(UINT uFormat, HANDLE hMem);

// Global variables to store original function addresses
GetClipboardData_t g_originalGetClipboardData = nullptr;
OpenClipboard_t g_originalOpenClipboard = nullptr;
CloseClipboard_t g_originalCloseClipboard = nullptr;
SetClipboardData_t g_originalSetClipboardData = nullptr;

// Function to get current process name
string getCurrentProcessName()
{
    char processName[256];
    HMODULE hModule = NULL;
    DWORD cbNeeded;

    // Use GetModuleFileName instead of EnumProcessModules for simplicity
    if (GetModuleFileNameA(NULL, processName, sizeof(processName)))
    {
        string fullPath = string(processName);
        size_t pos = fullPath.find_last_of("\\/");
        if (pos != string::npos)
        {
            return fullPath.substr(pos + 1);
        }
        return fullPath;
    }

    return "Unknown";
}

// Function to check if process is legitimate
bool isLegitimateApplication()
{
    string processName = getCurrentProcessName();

    // List of known legitimate applications (you would expand this list)
    vector<string> legitimateApps = {
        "notepad.exe",
        "calc.exe",
        "explorer.exe",
        "chrome.exe",
        "firefox.exe",
        "word.exe",
        "excel.exe"
    };

    for (const auto& app : legitimateApps)
    {
        if (processName.find(app) != string::npos)
        {
            return true;
        }
    }

    // If it's not in our known list, assume it might be malware
    return false;
}

// Hooked GetClipboardData function
HGLOBAL WINAPI MyGetClipboardData(UINT uFormat)
{
    string currentProcess = getCurrentProcessName();
    cout << "[Security] Clipboard access attempt by: " << currentProcess << endl;

    if (currentProcess.find("ClipboardLogger") != string::npos ||
        currentProcess.find("malware") != string::npos ||
        currentProcess.find("test") != string::npos)
    {
        cout << "[Security] Deception activated - returning decoy data" << endl;

        std::string decoy = decoyClipboardData[dist(gen) % decoyClipboardData.size()];
        size_t size = decoy.size() + 1;

        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, size);
        if (hMem)
        {
            void* pMem = GlobalLock(hMem);
            if (pMem)
            {
                memcpy(pMem, decoy.c_str(), size);
                GlobalUnlock(hMem);
                return hMem;
            }
            GlobalFree(hMem);
        }

        return nullptr; // fallback if something fails
    }

    if (g_originalGetClipboardData)
        return g_originalGetClipboardData(uFormat);

    return nullptr;
}


// Hooked OpenClipboard function
BOOL WINAPI MyOpenClipboard(HWND hWndNewOwner)
{
    cout << "[Security] OpenClipboard called" << endl;

    if (g_originalOpenClipboard)
    {
        return g_originalOpenClipboard(hWndNewOwner);
    }

    return FALSE;
}

// Hooked CloseClipboard function
BOOL WINAPI MyCloseClipboard(void)
{
    cout << "[Security] CloseClipboard called" << endl;

    if (g_originalCloseClipboard)
    {
        return g_originalCloseClipboard();
    }

    return FALSE;
}

// Hooked SetClipboardData function
HGLOBAL WINAPI MySetClipboardData(UINT uFormat, HANDLE hMem)
{
    cout << "[Security] SetClipboardData called" << endl;

    if (g_originalSetClipboardData)
    {
        return g_originalSetClipboardData(uFormat, hMem);
    }

    return NULL;
}

// Hook: CreateFileW - Example of additional hooking for demonstration
HANDLE WINAPI myCreateFileWHook(
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile)
{
    wcout << L"[Hook] CreateFileW intercepted: " << lpFileName << endl;
    return CreateFileW(lpFileName,
        dwDesiredAccess,
        dwShareMode,
        lpSecurityAttributes,
        dwCreationDisposition,
        dwFlagsAndAttributes,
        hTemplateFile);
}

// Deceptive GetAsyncKeyState with selective printing
SHORT WINAPI myGetAsyncKeyStateHook(int vKey)
{
    SHORT actualState = GetAsyncKeyState(vKey);
    int chance = dist(gen);
    if (chance < 20)
    {
        actualState ^= 0x8000; // Flip key pressed bit
        cout << "[Deception] Modified GetAsyncKeyState for VK: " << vKey << endl;
    }
    return actualState;
}

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
    cout << "[*] Injection started." << endl;

    // Initialize hook trace info
    HOOK_TRACE_INFO hGetClipboardDataHook = { NULL };
    HOOK_TRACE_INFO hOpenClipboardHook = { NULL };
    HOOK_TRACE_INFO hCloseClipboardHook = { NULL };
    HOOK_TRACE_INFO hSetClipboardDataHook = { NULL };
    HOOK_TRACE_INFO hCreateFileHook = { NULL };
    HOOK_TRACE_INFO hAsyncKeyHook = { NULL };

    // Get original function addresses
    FARPROC getClipboardAddr = GetProcAddress(GetModuleHandle(TEXT("user32")), "GetClipboardData");
    FARPROC openClipboardAddr = GetProcAddress(GetModuleHandle(TEXT("user32")), "OpenClipboard");
    FARPROC closeClipboardAddr = GetProcAddress(GetModuleHandle(TEXT("user32")), "CloseClipboard");
    FARPROC setClipboardDataAddr = GetProcAddress(GetModuleHandle(TEXT("user32")), "SetClipboardData");
    FARPROC createFileAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "CreateFileW");
    FARPROC asyncKeyAddr = GetProcAddress(GetModuleHandle(TEXT("user32")), "GetAsyncKeyState");

    // Store original function pointers
    if (getClipboardAddr)
        g_originalGetClipboardData = (GetClipboardData_t)getClipboardAddr;

    if (openClipboardAddr)
        g_originalOpenClipboard = (OpenClipboard_t)openClipboardAddr;

    if (closeClipboardAddr)
        g_originalCloseClipboard = (CloseClipboard_t)closeClipboardAddr;

    if (setClipboardDataAddr)
        g_originalSetClipboardData = (SetClipboardData_t)setClipboardDataAddr;

    // Install hooks for clipboard operations
    if (getClipboardAddr)
        LhInstallHook(getClipboardAddr, MyGetClipboardData, nullptr, &hGetClipboardDataHook);

    if (openClipboardAddr)
        LhInstallHook(openClipboardAddr, MyOpenClipboard, nullptr, &hOpenClipboardHook);

    if (closeClipboardAddr)
        LhInstallHook(closeClipboardAddr, MyCloseClipboard, nullptr, &hCloseClipboardHook);

    if (setClipboardDataAddr)
        LhInstallHook(setClipboardDataAddr, MySetClipboardData, nullptr, &hSetClipboardDataHook);

    // Install additional hooks for demonstration
    if (createFileAddr)
        LhInstallHook(createFileAddr, myCreateFileWHook, nullptr, &hCreateFileHook);

    if (asyncKeyAddr)
        LhInstallHook(asyncKeyAddr, myGetAsyncKeyStateHook, nullptr, &hAsyncKeyHook);

    // Enable all hooks
    ULONG ACLEntries[1] = { 0 };

    if (getClipboardAddr)
        LhSetExclusiveACL(ACLEntries, 1, &hGetClipboardDataHook);

    if (openClipboardAddr)
        LhSetExclusiveACL(ACLEntries, 1, &hOpenClipboardHook);

    if (closeClipboardAddr)
        LhSetExclusiveACL(ACLEntries, 1, &hCloseClipboardHook);

    if (setClipboardDataAddr)
        LhSetExclusiveACL(ACLEntries, 1, &hSetClipboardDataHook);

    if (createFileAddr)
        LhSetExclusiveACL(ACLEntries, 1, &hCreateFileHook);

    if (asyncKeyAddr)
        LhSetExclusiveACL(ACLEntries, 1, &hAsyncKeyHook);

    cout << "[*] All hooks installed successfully." << endl;
}