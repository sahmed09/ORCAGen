#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <easyhook.h>
#include <random>
#include <chrono>
#include <unordered_set>
#pragma comment(lib, "EasyHook32.lib")

using namespace std;

// Random generator
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dist(0, 99);

// Track legitimate screen capture requests
struct CaptureContext {
    DWORD threadId;
    chrono::steady_clock::time_point timestamp;
};

std::unordered_set<DWORD> legitimateThreads;
bool hookActive = true;

// Original BitBlt function pointer
typedef BOOL(WINAPI* BitBlt_t)(HDC, int, int, int, int, HDC, int, int, DWORD);
BitBlt_t OriginalBitBlt = nullptr;

// Helper function to check if the capture request is from the monitored process
bool IsSuspiciousScreenCapture(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight,
    HDC hdcSrc, int nXSrc, int nYSrc, DWORD dwRop)
{
    // Check if this is a full or large screen capture
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // If capturing a large portion of the screen (>50% of total area)
    float captureArea = (float)(nWidth * nHeight);
    float screenArea = (float)(screenWidth * screenHeight);
    float captureRatio = captureArea / screenArea;

    // Suspicious if capturing more than 50% of screen
    if (captureRatio > 0.5f) {
        return true;
    }

    // Check if this is a SRCCOPY operation (commonly used for screenshots)
    if (dwRop == SRCCOPY && nWidth >= screenWidth * 0.7f && nHeight >= screenHeight * 0.7f) {
        return true;
    }

    return false;
}

// Hook: BitBlt - Intercept screen capture operations
BOOL WINAPI myBitBltHook(
    HDC hdcDest,
    int nXDest,
    int nYDest,
    int nWidth,
    int nHeight,
    HDC hdcSrc,
    int nXSrc,
    int nYSrc,
    DWORD dwRop)
{
    DWORD currentThreadId = GetCurrentThreadId();

    // Check if this is a legitimate thread (whitelisted)
    if (legitimateThreads.find(currentThreadId) != legitimateThreads.end()) {
        // Allow legitimate captures to proceed
        return OriginalBitBlt(hdcDest, nXDest, nYDest, nWidth, nHeight,
            hdcSrc, nXSrc, nYSrc, dwRop);
    }

    // Check if this looks like a screenshot capture
    if (IsSuspiciousScreenCapture(hdcDest, nXDest, nYDest, nWidth, nHeight,
        hdcSrc, nXSrc, nYSrc, dwRop)) {
        cout << "[Hook] BitBlt intercepted - Suspicious screen capture detected!" << endl;
        cout << "       Dimensions: " << nWidth << "x" << nHeight << endl;

        // FAKE SUCCESS: Return TRUE to make the malware think it succeeded
        // but don't actually perform the BitBlt operation

        // Optional: Fill with a decoy image (solid color or noise)
        // This makes the malware save a fake image instead of real screen content
        HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 0)); // Black screen
        RECT rect = { nXDest, nYDest, nXDest + nWidth, nYDest + nHeight };
        FillRect(hdcDest, &rect, hBrush);
        DeleteObject(hBrush);

        cout << "[Deception] Fake success returned - Screenshot blocked!" << endl;

        // Return TRUE to simulate success
        return TRUE;
    }

    // For normal BitBlt operations (small regions, UI elements, etc.), allow them
    return OriginalBitBlt(hdcDest, nXDest, nYDest, nWidth, nHeight,
        hdcSrc, nXSrc, nYSrc, dwRop);
}

// Hook: CreateFileW
HANDLE WINAPI myCreateFileWHook(
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile)
{
    // Check if creating a screenshot file
    wstring filename(lpFileName);
    if (filename.find(L"screenshot_") != wstring::npos && filename.find(L".png") != wstring::npos) {
        wcout << L"[Hook] Screenshot file creation detected: " << lpFileName << endl;
        wcout << L"[Deception] Allowing file creation (will contain fake data)" << endl;
    }

    return CreateFileW(lpFileName,
        dwDesiredAccess,
        dwShareMode,
        lpSecurityAttributes,
        dwCreationDisposition,
        dwFlagsAndAttributes,
        hTemplateFile);
}

// Deceptive GetAsyncKeyState with selective modification
SHORT WINAPI myGetAsyncKeyStateHook(int vKey)
{
    SHORT actualState = GetAsyncKeyState(vKey);
    int chance = dist(gen);

    if (chance < 20) {
        actualState ^= 0x8000;  // Flip key pressed bit
        cout << "[Deception] Modified GetAsyncKeyState for VK: " << vKey << endl;
    }

    return actualState;
}

// Add a thread to the whitelist (for legitimate captures)
extern "C" void __declspec(dllexport) __stdcall AddLegitimateThread(DWORD threadId)
{
    legitimateThreads.insert(threadId);
    cout << "[*] Thread " << threadId << " added to legitimate list" << endl;
}

// Remove a thread from the whitelist
extern "C" void __declspec(dllexport) __stdcall RemoveLegitimateThread(DWORD threadId)
{
    legitimateThreads.erase(threadId);
    cout << "[*] Thread " << threadId << " removed from legitimate list" << endl;
}

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
    cout << "[*] Active Defense Injection started." << endl;
    cout << "[*] BitBlt hooking enabled for screenshot protection." << endl;

    HOOK_TRACE_INFO hBitBltHook = { NULL };
    HOOK_TRACE_INFO hCreateFileHook = { NULL };
    HOOK_TRACE_INFO hAsyncKeyHook = { NULL };

    // Get original BitBlt address
    FARPROC bitBltAddr = GetProcAddress(GetModuleHandle(TEXT("gdi32")), "BitBlt");
    if (bitBltAddr == nullptr) {
        cout << "[-] Failed to get BitBlt address!" << endl;
        return;
    }
    OriginalBitBlt = (BitBlt_t)bitBltAddr;

    // Install BitBlt hook
    NTSTATUS status = LhInstallHook(bitBltAddr, myBitBltHook, nullptr, &hBitBltHook);
    if (status != 0) {
        cout << "[-] Failed to install BitBlt hook. Error: " << status << endl;
        return;
    }
    cout << "[+] BitBlt hook installed successfully." << endl;

    // Install CreateFileW hook
    FARPROC createFileAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "CreateFileW");
    LhInstallHook(createFileAddr, myCreateFileWHook, nullptr, &hCreateFileHook);
    cout << "[+] CreateFileW hook installed successfully." << endl;

    // Install GetAsyncKeyState hook
    FARPROC asyncKeyAddr = GetProcAddress(GetModuleHandle(TEXT("user32")), "GetAsyncKeyState");
    LhInstallHook(asyncKeyAddr, myGetAsyncKeyStateHook, nullptr, &hAsyncKeyHook);
    cout << "[+] GetAsyncKeyState hook installed successfully." << endl;

    // Enable all hooks for all threads (ACL entry 0 = all threads)
    ULONG ACLEntries[1] = { 0 };
    LhSetExclusiveACL(ACLEntries, 1, &hBitBltHook);
    LhSetExclusiveACL(ACLEntries, 1, &hCreateFileHook);
    LhSetExclusiveACL(ACLEntries, 1, &hAsyncKeyHook);

    cout << "[*] All hooks activated. Monitoring screen capture attempts..." << endl;
    cout << "[*] Legitimate captures can be whitelisted via AddLegitimateThread()." << endl;
}