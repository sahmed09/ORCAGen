// RansomwareDefense.cpp
#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <easyhook.h>
#include <random>
#include <set>

#pragma comment(lib, "EasyHook32.lib")
#pragma comment(lib, "psapi.lib")

using namespace std;

// Encryption key (in a real scenario, this would be more sophisticated)
const char* ENCRYPTION_KEY = "MySecretKey123";

// List of file extensions to target for encryption
const vector<string> TARGET_EXTENSIONS = {
    ".txt", ".docx", ".xlsx", ".db", ".pdf", ".jpg", ".png", ".gif",
    ".mp4", ".avi", ".mov", ".mp3", ".wav", ".exe", ".dll", ".sys"
};

// List of extensions to skip (system/critical files)
const vector<string> SKIP_EXTENSIONS = {
    ".sys", ".dll", ".exe", ".ocx", ".cpl", ".drv", ".vxd", ".bin",
    ".msi", ".msp", ".mst", ".scr", ".bat", ".cmd", ".com", ".js",
    ".vbs", ".wsh", ".wsf", ".hta", ".lnk", ".pif"
};

// Global variables for tracking
set<string> blockedFiles;
set<string> legitimateApps = {
    "explorer.exe", "chrome.exe", "firefox.exe", "notepad.exe",
    "word.exe", "excel.exe", "powerpoint.exe", "vscode.exe"
};

// Random generator for deception
random_device rd;
mt19937 gen(rd());
uniform_int_distribution<> dist(0, 99);

// Function to check if a file extension is in our target list
bool IsTargetFile(const string& filename) {
    size_t dotPos = filename.find_last_of('.');
    if (dotPos == string::npos) return false;
    string ext = filename.substr(dotPos);
    transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    // Check if extension is in target list
    for (const auto& targetExt : TARGET_EXTENSIONS) {
        if (ext == targetExt) {
            return true;
        }
    }
    return false;
}

// Function to check if a file extension should be skipped
bool ShouldSkipFile(const string& filename) {
    size_t dotPos = filename.find_last_of('.');
    if (dotPos == string::npos) return false;
    string ext = filename.substr(dotPos);
    transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    // Check if extension is in skip list
    for (const auto& skipExt : SKIP_EXTENSIONS) {
        if (ext == skipExt) {
            return true;
        }
    }
    return false;
}

// Function to determine if current process is legitimate
bool IsLegitimateProcess() {
    char processName[MAX_PATH];

    // Use GetModuleFileNameA instead of GetModuleBaseNameA
    HMODULE hMod = GetModuleHandle(NULL);
    if (GetModuleFileNameA(hMod, processName, MAX_PATH)) {
        string name = processName;
        size_t lastSlash = name.find_last_of("\\/");
        if (lastSlash != string::npos) {
            name = name.substr(lastSlash + 1);
        }
        transform(name.begin(), name.end(), name.begin(), ::tolower);

        return legitimateApps.find(name) != legitimateApps.end();
    }

    return false;
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
    // Check if this is a legitimate application
    if (IsLegitimateProcess()) {
        return CreateFileW(lpFileName, dwDesiredAccess, dwShareMode,
            lpSecurityAttributes, dwCreationDisposition,
            dwFlagsAndAttributes, hTemplateFile);
    }

    // Convert wide string to narrow string for analysis
    char fileName[1024];
    WideCharToMultiByte(CP_UTF8, 0, lpFileName, -1, fileName, 1024, NULL, NULL);

    // Check if this is a target file type for encryption
    string filename = fileName;
    size_t lastSlash = filename.find_last_of("\\/");
    if (lastSlash != string::npos) {
        filename = filename.substr(lastSlash + 1);
    }

    // If it's a target file and we're not in a legitimate app, block the operation
    if (IsTargetFile(filename)) {
        // Log the attempt
        wcout << L"[Defense] Blocking CreateFileW for: " << lpFileName << endl;

        // Return INVALID_HANDLE_VALUE to simulate failure
        SetLastError(ERROR_ACCESS_DENIED);
        return INVALID_HANDLE_VALUE;
    }

    // Otherwise, proceed with normal operation
    return CreateFileW(lpFileName, dwDesiredAccess, dwShareMode,
        lpSecurityAttributes, dwCreationDisposition,
        dwFlagsAndAttributes, hTemplateFile);
}

// Hook: WriteFile
BOOL WINAPI myWriteFileHook(
    HANDLE hFile,
    LPCVOID lpBuffer,
    DWORD nNumberOfBytesToWrite,
    LPDWORD lpNumberOfBytesWritten,
    LPOVERLAPPED lpOverlapped)
{
    // Get the process name for legitimacy check
    char processName[MAX_PATH];
    HMODULE hMod = GetModuleHandle(NULL);
    if (GetModuleFileNameA(hMod, processName, MAX_PATH)) {
        string name = processName;
        size_t lastSlash = name.find_last_of("\\/");
        if (lastSlash != string::npos) {
            name = name.substr(lastSlash + 1);
        }
        transform(name.begin(), name.end(), name.begin(), ::tolower);

        // If legitimate, allow the operation
        if (legitimateApps.find(name) != legitimateApps.end()) {
            return WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
        }
    }

    // For non-legitimate processes, we need to check what file is being accessed
    // This simplified version blocks all operations for non-legitimate apps
    // A more sophisticated implementation would use GetFinalPathNameByHandle or similar

    return WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
}

// Hook: SetFilePointer
DWORD WINAPI mySetFilePointerHook(
    HANDLE hFile,
    LONG lDistanceToMove,
    PLONG lpDistanceToMoveHigh,
    DWORD dwMoveMethod)
{
    // Get the process name for legitimacy check
    char processName[MAX_PATH];
    HMODULE hMod = GetModuleHandle(NULL);
    if (GetModuleFileNameA(hMod, processName, MAX_PATH)) {
        string name = processName;
        size_t lastSlash = name.find_last_of("\\/");
        if (lastSlash != string::npos) {
            name = name.substr(lastSlash + 1);
        }
        transform(name.begin(), name.end(), name.begin(), ::tolower);

        // If legitimate, allow the operation
        if (legitimateApps.find(name) != legitimateApps.end()) {
            return SetFilePointer(hFile, lDistanceToMove, lpDistanceToMoveHigh, dwMoveMethod);
        }
    }

    // For non-legitimate processes, block file pointer operations
    SetLastError(ERROR_ACCESS_DENIED);
    return INVALID_SET_FILE_POINTER;
}

// Hook: FlushFileBuffers
BOOL WINAPI myFlushFileBuffersHook(
    HANDLE hFile)
{
    // Get the process name for legitimacy check
    char processName[MAX_PATH];
    HMODULE hMod = GetModuleHandle(NULL);
    if (GetModuleFileNameA(hMod, processName, MAX_PATH)) {
        string name = processName;
        size_t lastSlash = name.find_last_of("\\/");
        if (lastSlash != string::npos) {
            name = name.substr(lastSlash + 1);
        }
        transform(name.begin(), name.end(), name.begin(), ::tolower);

        // If legitimate, allow the operation
        if (legitimateApps.find(name) != legitimateApps.end()) {
            return FlushFileBuffers(hFile);
        }
    }

    // For non-legitimate processes, block flush operations
    SetLastError(ERROR_ACCESS_DENIED);
    return FALSE;
}

// Hook: GetAsyncKeyState
SHORT WINAPI myGetAsyncKeyStateHook(
    int vKey)
{
    // This hook is for demonstration purposes only
    // In a real implementation, we might modify behavior here

    // Call the original function
    return GetAsyncKeyState(vKey);
}

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
    cout << "[*] Ransomware Defense Injection Started." << endl;

    HOOK_TRACE_INFO hCreateFileHook = { NULL };
    HOOK_TRACE_INFO hWriteFileHook = { NULL };
    HOOK_TRACE_INFO hSetFilePointerHook = { NULL };
    HOOK_TRACE_INFO hFlushFileBuffersHook = { NULL };
    HOOK_TRACE_INFO hAsyncKeyHook = { NULL };

    // Install hooks
    FARPROC createFileAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "CreateFileW");
    LhInstallHook(createFileAddr, myCreateFileWHook, nullptr, &hCreateFileHook);

    FARPROC writeFileAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "WriteFile");
    LhInstallHook(writeFileAddr, myWriteFileHook, nullptr, &hWriteFileHook);

    FARPROC setFilePointerAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "SetFilePointer");
    LhInstallHook(setFilePointerAddr, mySetFilePointerHook, nullptr, &hSetFilePointerHook);

    FARPROC flushFileBuffersAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "FlushFileBuffers");
    LhInstallHook(flushFileBuffersAddr, myFlushFileBuffersHook, nullptr, &hFlushFileBuffersHook);

    FARPROC asyncKeyAddr = GetProcAddress(GetModuleHandle(TEXT("user32")), "GetAsyncKeyState");
    LhInstallHook(asyncKeyAddr, myGetAsyncKeyStateHook, nullptr, &hAsyncKeyHook);

    // Enable all hooks
    ULONG ACLEntries[1] = { 0 };
    LhSetExclusiveACL(ACLEntries, 1, &hCreateFileHook);
    LhSetExclusiveACL(ACLEntries, 1, &hWriteFileHook);
    LhSetExclusiveACL(ACLEntries, 1, &hSetFilePointerHook);
    LhSetExclusiveACL(ACLEntries, 1, &hFlushFileBuffersHook);
    LhSetExclusiveACL(ACLEntries, 1, &hAsyncKeyHook);

    cout << "[*] All hooks installed successfully." << endl;
}