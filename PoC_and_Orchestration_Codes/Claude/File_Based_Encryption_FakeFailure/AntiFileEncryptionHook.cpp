#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <easyhook.h>
#include <shlwapi.h>
#pragma comment(lib, "EasyHook32.lib")
#pragma comment(lib, "shlwapi.lib")

using namespace std;

// ============================================================================
// CONFIGURATION AND DETECTION PARAMETERS
// ============================================================================

// Protected file extensions (ransomware targets)
const unordered_set<wstring> PROTECTED_EXTENSIONS = {
    L".docx", L".xlsx", L".pdf", L".db", L".txt", L".jpg", L".png",
    L".ppt", L".pptx", L".xls", L".doc", L".zip", L".rar"
};

// Aggressive file handle tracking
struct FileHandleInfo {
    wstring filename;
    DWORD openAccess;
    bool hasRead;
    bool hasSeekToBegin;
    int writeAttempts;
    DWORD openTime;
    BYTE firstReadByte;
    BYTE lastWriteByte;
};

unordered_map<HANDLE, FileHandleInfo> handleTracker;
CRITICAL_SECTION trackerLock;

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

void InitializeTracker() {
    InitializeCriticalSection(&trackerLock);
}

bool IsProtectedFileType(LPCWSTR lpFileName) {
    if (!lpFileName) return false;

    wstring filename(lpFileName);
    size_t dotPos = filename.find_last_of(L'.');

    if (dotPos == wstring::npos) return false;

    wstring ext = filename.substr(dotPos);
    transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    return PROTECTED_EXTENSIONS.find(ext) != PROTECTED_EXTENSIONS.end();
}

bool IsRansomwareBehavior(HANDLE hFile) {
    EnterCriticalSection(&trackerLock);

    auto it = handleTracker.find(hFile);
    if (it == handleTracker.end()) {
        LeaveCriticalSection(&trackerLock);
        return false;
    }

    FileHandleInfo& info = it->second;

    // Ransomware pattern: Read file + Seek to beginning + Write back
    bool isSuspicious = info.hasRead && info.hasSeekToBegin && (info.writeAttempts > 0);

    // Additional check: XOR pattern (first read byte XOR'd with write byte)
    if (isSuspicious && info.firstReadByte != 0 && info.lastWriteByte != 0) {
        // Check if XOR pattern exists (simplified detection)
        BYTE xorKey = info.firstReadByte ^ info.lastWriteByte;
        if (xorKey == 0xAA || xorKey == 0xFF || xorKey == 0x55) {
            wcout << L"[CRITICAL] XOR encryption key detected: 0x" << hex << (int)xorKey << dec << endl;
            LeaveCriticalSection(&trackerLock);
            return true;
        }
    }

    LeaveCriticalSection(&trackerLock);
    return isSuspicious;
}

// ============================================================================
// HOOKED FUNCTIONS IMPLEMENTATION
// ============================================================================

HANDLE WINAPI myCreateFileWHook(
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile)
{
    if (lpFileName) {
        wcout << L"[Hook] CreateFileW: " << lpFileName << endl;

        // Check if this is a protected file type with WRITE access
        if (IsProtectedFileType(lpFileName) && (dwDesiredAccess & GENERIC_WRITE)) {
            wcout << L"[Monitor] Protected file opened for WRITE: " << lpFileName << endl;
        }
    }

    HANDLE hFile = CreateFileW(
        lpFileName,
        dwDesiredAccess,
        dwShareMode,
        lpSecurityAttributes,
        dwCreationDisposition,
        dwFlagsAndAttributes,
        hTemplateFile);

    // Track this handle if successful and it's a protected file
    if (hFile != INVALID_HANDLE_VALUE && lpFileName && IsProtectedFileType(lpFileName)) {
        EnterCriticalSection(&trackerLock);

        FileHandleInfo info;
        info.filename = lpFileName;
        info.openAccess = dwDesiredAccess;
        info.hasRead = false;
        info.hasSeekToBegin = false;
        info.writeAttempts = 0;
        info.openTime = GetTickCount();
        info.firstReadByte = 0;
        info.lastWriteByte = 0;

        handleTracker[hFile] = info;

        LeaveCriticalSection(&trackerLock);
        wcout << L"[Track] Handle registered: " << hFile << L" for " << lpFileName << endl;
    }

    return hFile;
}

BOOL WINAPI myReadFileHook(
    HANDLE hFile,
    LPVOID lpBuffer,
    DWORD nNumberOfBytesToRead,
    LPDWORD lpNumberOfBytesRead,
    LPOVERLAPPED lpOverlapped)
{
    BOOL result = ReadFile(hFile, lpBuffer, nNumberOfBytesToRead,
        lpNumberOfBytesRead, lpOverlapped);

    if (result && lpBuffer && lpNumberOfBytesRead && *lpNumberOfBytesRead > 0) {
        EnterCriticalSection(&trackerLock);

        auto it = handleTracker.find(hFile);
        if (it != handleTracker.end()) {
            it->second.hasRead = true;
            it->second.firstReadByte = ((BYTE*)lpBuffer)[0];

            wcout << L"[Track] ReadFile on tracked handle: " << hFile
                << L" (" << it->second.filename << L")" << endl;
        }

        LeaveCriticalSection(&trackerLock);
    }

    return result;
}

DWORD WINAPI mySetFilePointerHook(
    HANDLE hFile,
    LONG lDistanceToMove,
    PLONG lpDistanceToMoveHigh,
    DWORD dwMoveMethod)
{
    // Detect seek to beginning (ransomware rewrite pattern)
    if (dwMoveMethod == FILE_BEGIN && lDistanceToMove == 0) {
        EnterCriticalSection(&trackerLock);

        auto it = handleTracker.find(hFile);
        if (it != handleTracker.end()) {
            it->second.hasSeekToBegin = true;
            wcout << L"[ALERT] Seek-to-begin detected on: " << it->second.filename << endl;

            // If we've already read the file, this is highly suspicious
            if (it->second.hasRead) {
                wcout << L"[CRITICAL] Read + Seek-to-begin = Ransomware pattern!" << endl;
            }
        }

        LeaveCriticalSection(&trackerLock);
    }

    return SetFilePointer(hFile, lDistanceToMove, lpDistanceToMoveHigh, dwMoveMethod);
}

BOOL WINAPI myWriteFileHook(
    HANDLE hFile,
    LPCVOID lpBuffer,
    DWORD nNumberOfBytesToWrite,
    LPDWORD lpNumberOfBytesWritten,
    LPOVERLAPPED lpOverlapped)
{
    EnterCriticalSection(&trackerLock);

    auto it = handleTracker.find(hFile);
    if (it != handleTracker.end()) {
        FileHandleInfo& info = it->second;
        info.writeAttempts++;

        if (lpBuffer && nNumberOfBytesToWrite > 0) {
            info.lastWriteByte = ((BYTE*)lpBuffer)[0];
        }

        wcout << L"[Hook] WriteFile attempt #" << info.writeAttempts
            << L" on: " << info.filename << endl;

        // Check for ransomware behavior
        if (IsRansomwareBehavior(hFile)) {
            wcout << L"[BLOCKED] *** RANSOMWARE DETECTED *** File: " << info.filename << endl;
            wcout << L"[BLOCKED] Returning FakeFailure to malware" << endl;

            LeaveCriticalSection(&trackerLock);

            // Return fake failure
            if (lpNumberOfBytesWritten) {
                *lpNumberOfBytesWritten = 0;
            }
            SetLastError(ERROR_WRITE_FAULT);
            return FALSE;
        }

        // Additional aggressive check: If read + seek + write on protected file, BLOCK
        if (info.hasRead && info.hasSeekToBegin) {
            wcout << L"[BLOCKED] *** Suspicious pattern on protected file *** " << info.filename << endl;
            LeaveCriticalSection(&trackerLock);

            if (lpNumberOfBytesWritten) {
                *lpNumberOfBytesWritten = 0;
            }
            SetLastError(ERROR_ACCESS_DENIED);
            return FALSE;
        }
    }

    LeaveCriticalSection(&trackerLock);

    // Allow legitimate writes
    return WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite,
        lpNumberOfBytesWritten, lpOverlapped);
}

BOOL WINAPI myCloseHandleHook(HANDLE hObject)
{
    // Clean up tracking for this handle
    EnterCriticalSection(&trackerLock);

    auto it = handleTracker.find(hObject);
    if (it != handleTracker.end()) {
        wcout << L"[Track] Closing handle: " << hObject
            << L" (" << it->second.filename << L")" << endl;
        handleTracker.erase(it);
    }

    LeaveCriticalSection(&trackerLock);

    return CloseHandle(hObject);
}

BOOL WINAPI myFlushFileBuffersHook(HANDLE hFile)
{
    EnterCriticalSection(&trackerLock);

    auto it = handleTracker.find(hFile);
    if (it != handleTracker.end()) {
        wcout << L"[Hook] FlushFileBuffers on: " << it->second.filename << endl;

        // If ransomware behavior detected, block the flush too
        if (IsRansomwareBehavior(hFile)) {
            wcout << L"[BLOCKED] Flush blocked for ransomware file: " << it->second.filename << endl;
            LeaveCriticalSection(&trackerLock);
            SetLastError(ERROR_WRITE_FAULT);
            return FALSE;
        }
    }

    LeaveCriticalSection(&trackerLock);

    return FlushFileBuffers(hFile);
}

// ============================================================================
// INJECTION ENTRY POINT
// ============================================================================

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
    cout << "========================================" << endl;
    cout << "[*] Ransomware Defense Hook ACTIVE" << endl;
    cout << "========================================" << endl;

    InitializeTracker();

    HOOK_TRACE_INFO hCreateFileHook = { NULL };
    HOOK_TRACE_INFO hReadFileHook = { NULL };
    HOOK_TRACE_INFO hWriteFileHook = { NULL };
    HOOK_TRACE_INFO hSetFilePointerHook = { NULL };
    HOOK_TRACE_INFO hFlushFileBuffersHook = { NULL };
    HOOK_TRACE_INFO hCloseHandleHook = { NULL };

    HMODULE hKernel32 = GetModuleHandle(TEXT("kernel32.dll"));

    FARPROC pCreateFileW = GetProcAddress(hKernel32, "CreateFileW");
    FARPROC pReadFile = GetProcAddress(hKernel32, "ReadFile");
    FARPROC pWriteFile = GetProcAddress(hKernel32, "WriteFile");
    FARPROC pSetFilePointer = GetProcAddress(hKernel32, "SetFilePointer");
    FARPROC pFlushFileBuffers = GetProcAddress(hKernel32, "FlushFileBuffers");
    FARPROC pCloseHandle = GetProcAddress(hKernel32, "CloseHandle");

    NTSTATUS status;

    status = LhInstallHook(pCreateFileW, myCreateFileWHook, nullptr, &hCreateFileHook);
    cout << "[Hook] CreateFileW: " << (status == 0 ? "SUCCESS" : "FAILED") << endl;

    status = LhInstallHook(pReadFile, myReadFileHook, nullptr, &hReadFileHook);
    cout << "[Hook] ReadFile: " << (status == 0 ? "SUCCESS" : "FAILED") << endl;

    status = LhInstallHook(pWriteFile, myWriteFileHook, nullptr, &hWriteFileHook);
    cout << "[Hook] WriteFile: " << (status == 0 ? "SUCCESS" : "FAILED") << endl;

    status = LhInstallHook(pSetFilePointer, mySetFilePointerHook, nullptr, &hSetFilePointerHook);
    cout << "[Hook] SetFilePointer: " << (status == 0 ? "SUCCESS" : "FAILED") << endl;

    status = LhInstallHook(pFlushFileBuffers, myFlushFileBuffersHook, nullptr, &hFlushFileBuffersHook);
    cout << "[Hook] FlushFileBuffers: " << (status == 0 ? "SUCCESS" : "FAILED") << endl;

    status = LhInstallHook(pCloseHandle, myCloseHandleHook, nullptr, &hCloseHandleHook);
    cout << "[Hook] CloseHandle: " << (status == 0 ? "SUCCESS" : "FAILED") << endl;

    ULONG ACLEntries[1] = { 0 };

    LhSetExclusiveACL(ACLEntries, 1, &hCreateFileHook);
    LhSetExclusiveACL(ACLEntries, 1, &hReadFileHook);
    LhSetExclusiveACL(ACLEntries, 1, &hWriteFileHook);
    LhSetExclusiveACL(ACLEntries, 1, &hSetFilePointerHook);
    LhSetExclusiveACL(ACLEntries, 1, &hFlushFileBuffersHook);
    LhSetExclusiveACL(ACLEntries, 1, &hCloseHandleHook);

    cout << "========================================" << endl;
    cout << "[SUCCESS] All hooks active" << endl;
    cout << "[*] Detection: Read + Seek + Write = BLOCK" << endl;
    cout << "[*] Protected: .docx .xlsx .db .txt etc." << endl;
    cout << "========================================" << endl;
}