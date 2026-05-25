#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <mutex>
#include <algorithm>
#include <cctype>
#include <easyhook.h>

#pragma comment(lib, "EasyHook32.lib")

using namespace std;

// ============================================================================
// CONFIGURATION
// ============================================================================

// Target extensions to protect
const vector<wstring> PROTECTED_EXTENSIONS = {
    L".docx", L".xlsx", L".db", L".txt", L".pdf", L".jpg", L".png"
};

// Thresholds for ransomware detection
const DWORD RAPID_WRITE_THRESHOLD = 5;           // Files written in quick succession
const DWORD RAPID_WRITE_TIMEFRAME_MS = 2000;    // Time window for detection
const DWORD MIN_XOR_PATTERN_THRESHOLD = 80;     // % of bytes that match XOR pattern

// ============================================================================
// GLOBAL STATE TRACKING
// ============================================================================

struct FileOperationState {
    HANDLE fakeHandle;
    wstring filename;
    DWORD fileSize;
    DWORD bytesWritten;
    bool isProtected;
    bool isSuspicious;
    DWORD creationTime;
};

map<HANDLE, FileOperationState> g_FileHandles;
vector<DWORD> g_RecentWriteTimes;
mutex g_StateMutex;
DWORD g_SuspiciousActivityCount = 0;

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

wstring ToLower(const wstring& str) {
    wstring result = str;
    for (size_t i = 0; i < result.length(); i++) {
        result[i] = towlower(result[i]);
    }
    return result;
}

bool HasProtectedExtension(const wstring& filename) {
    for (const auto& ext : PROTECTED_EXTENSIONS) {
        if (filename.length() >= ext.length()) {
            wstring fileExt = filename.substr(filename.length() - ext.length());
            // Case-insensitive comparison
            wstring fileExtLower = ToLower(fileExt);
            wstring extLower = ToLower(ext);
            if (fileExtLower == extLower) {
                return true;
            }
        }
    }
    return false;
}

bool DetectXORPattern(const BYTE* buffer, DWORD size) {
    if (size < 100) return false;

    // Sample bytes to detect XOR encryption pattern
    map<BYTE, DWORD> xorKeyVotes;
    const DWORD sampleSize = min(size, (DWORD)1000);

    // Check for common XOR patterns
    for (DWORD i = 0; i < sampleSize - 1; i++) {
        BYTE potentialKey = buffer[i] ^ 0x00; // Common plaintext byte
        xorKeyVotes[potentialKey]++;
    }

    // Check entropy (XOR encrypted data has high entropy)
    DWORD highEntropyBytes = 0;
    for (DWORD i = 0; i < sampleSize; i++) {
        if (buffer[i] > 0x20 && buffer[i] < 0x7E) {
            // Printable ASCII range
        }
        else {
            highEntropyBytes++;
        }
    }

    DWORD entropyPercent = (highEntropyBytes * 100) / sampleSize;
    return entropyPercent > MIN_XOR_PATTERN_THRESHOLD;
}

bool IsRapidFileActivity() {
    lock_guard<mutex> lock(g_StateMutex);

    DWORD currentTime = GetTickCount();

    // Clean old entries
    g_RecentWriteTimes.erase(
        remove_if(g_RecentWriteTimes.begin(), g_RecentWriteTimes.end(),
            [currentTime](DWORD time) {
                return (currentTime - time) > RAPID_WRITE_TIMEFRAME_MS;
            }),
        g_RecentWriteTimes.end()
                );

    g_RecentWriteTimes.push_back(currentTime);

    return g_RecentWriteTimes.size() >= RAPID_WRITE_THRESHOLD;
}

HANDLE GenerateFakeHandle() {
    // Generate a unique fake handle (use high bit to mark as fake)
    static DWORD fakeHandleCounter = 0x80000000;
    return (HANDLE)(ULONG_PTR)(fakeHandleCounter++);
}

// ============================================================================
// HOOKED FUNCTIONS
// ============================================================================

// Original function pointers
typedef HANDLE(WINAPI* CreateFileA_t)(LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
typedef HANDLE(WINAPI* CreateFileW_t)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
typedef BOOL(WINAPI* WriteFile_t)(HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED);
typedef BOOL(WINAPI* ReadFile_t)(HANDLE, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
typedef DWORD(WINAPI* SetFilePointer_t)(HANDLE, LONG, PLONG, DWORD);
typedef BOOL(WINAPI* CloseHandle_t)(HANDLE);
typedef BOOL(WINAPI* FlushFileBuffers_t)(HANDLE);
typedef DWORD(WINAPI* GetFileSize_t)(HANDLE, LPDWORD);

// Hook: GetFileSize
DWORD WINAPI myGetFileSizeHook(HANDLE hFile, LPDWORD lpFileSizeHigh)
{
    lock_guard<mutex> lock(g_StateMutex);

    auto it = g_FileHandles.find(hFile);
    if (it != g_FileHandles.end() && it->second.isSuspicious) {
        wcout << L"[FAKESUCCESS] GetFileSize for: " << it->second.filename << endl;

        // Return a fake file size (1KB)
        if (lpFileSizeHigh != nullptr) {
            *lpFileSizeHigh = 0;
        }
        SetLastError(ERROR_SUCCESS);
        return 1024; // Fake 1KB file size
    }

    return GetFileSize(hFile, lpFileSizeHigh);
}

// Hook: CreateFileA
HANDLE WINAPI myCreateFileAHook(
    LPCSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile)
{
    // Convert to wide string for unified handling
    int wideLen = MultiByteToWideChar(CP_ACP, 0, lpFileName, -1, NULL, 0);
    wstring wideFilename(wideLen, 0);
    MultiByteToWideChar(CP_ACP, 0, lpFileName, -1, &wideFilename[0], wideLen);

    bool isProtected = HasProtectedExtension(wideFilename);
    bool isSuspicious = IsRapidFileActivity();

    // AGGRESSIVE MODE: Block all write access to protected files
    if (isProtected && (dwDesiredAccess & GENERIC_WRITE)) {
        wcout << L"[BLOCK] CreateFileA intercepted: " << wideFilename;
        if (isSuspicious) {
            wcout << L" (RAPID ACTIVITY DETECTED)";
        }
        wcout << endl;

        // Generate fake handle
        HANDLE fakeHandle = GenerateFakeHandle();

        lock_guard<mutex> lock(g_StateMutex);
        g_FileHandles[fakeHandle] = {
            fakeHandle,
            wideFilename,
            0,
            0,
            true,
            true,  // Always treat as suspicious for protected files
            GetTickCount()
        };

        g_SuspiciousActivityCount++;

        return fakeHandle;
    }

    // Allow legitimate file operations
    wcout << L"[ALLOW] CreateFileA: " << wideFilename << endl;
    return CreateFileA(lpFileName, dwDesiredAccess, dwShareMode,
        lpSecurityAttributes, dwCreationDisposition,
        dwFlagsAndAttributes, hTemplateFile);
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
    wstring filename(lpFileName);
    bool isProtected = HasProtectedExtension(filename);
    bool isSuspicious = IsRapidFileActivity();

    // AGGRESSIVE MODE: Block all write access to protected files
    if (isProtected && (dwDesiredAccess & GENERIC_WRITE)) {
        wcout << L"[BLOCK] CreateFileW intercepted: " << filename;
        if (isSuspicious) {
            wcout << L" (RAPID ACTIVITY DETECTED)";
        }
        wcout << endl;

        // Generate fake handle
        HANDLE fakeHandle = GenerateFakeHandle();

        lock_guard<mutex> lock(g_StateMutex);
        g_FileHandles[fakeHandle] = {
            fakeHandle,
            filename,
            0,
            0,
            true,
            true,  // Always treat as suspicious for protected files
            GetTickCount()
        };

        g_SuspiciousActivityCount++;

        return fakeHandle;
    }

    // Allow legitimate file operations
    wcout << L"[ALLOW] CreateFileW: " << filename << endl;
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
    lock_guard<mutex> lock(g_StateMutex);

    auto it = g_FileHandles.find(hFile);
    if (it != g_FileHandles.end() && it->second.isSuspicious) {
        // FakeSuccess: Pretend write succeeded
        wcout << L"[FAKESUCCESS] WriteFile blocked for: " << it->second.filename
            << L" (" << nNumberOfBytesToWrite << L" bytes)" << endl;

        // Additional XOR pattern detection
        if (nNumberOfBytesToWrite > 0 && lpBuffer != nullptr) {
            if (DetectXORPattern((const BYTE*)lpBuffer, nNumberOfBytesToWrite)) {
                wcout << L"[ALERT] XOR encryption pattern detected!" << endl;
            }
        }

        it->second.bytesWritten += nNumberOfBytesToWrite;

        if (lpNumberOfBytesWritten != nullptr) {
            *lpNumberOfBytesWritten = nNumberOfBytesToWrite;
        }

        SetLastError(ERROR_SUCCESS);
        return TRUE; // Return success without writing
    }

    // Allow legitimate writes
    return WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite,
        lpNumberOfBytesWritten, lpOverlapped);
}

// Hook: ReadFile
BOOL WINAPI myReadFileHook(
    HANDLE hFile,
    LPVOID lpBuffer,
    DWORD nNumberOfBytesToRead,
    LPDWORD lpNumberOfBytesRead,
    LPOVERLAPPED lpOverlapped)
{
    lock_guard<mutex> lock(g_StateMutex);

    auto it = g_FileHandles.find(hFile);
    if (it != g_FileHandles.end() && it->second.isSuspicious) {
        // FakeSuccess: Return fake data (original file content)
        wcout << L"[FAKESUCCESS] ReadFile intercepted for: " << it->second.filename << endl;

        // Fill with dummy data to simulate read
        if (lpBuffer != nullptr && nNumberOfBytesToRead > 0) {
            ZeroMemory(lpBuffer, nNumberOfBytesToRead);
        }

        if (lpNumberOfBytesRead != nullptr) {
            *lpNumberOfBytesRead = nNumberOfBytesToRead;
        }

        SetLastError(ERROR_SUCCESS);
        return TRUE;
    }

    return ReadFile(hFile, lpBuffer, nNumberOfBytesToRead,
        lpNumberOfBytesRead, lpOverlapped);
}

// Hook: SetFilePointer
DWORD WINAPI mySetFilePointerHook(
    HANDLE hFile,
    LONG lDistanceToMove,
    PLONG lpDistanceToMoveHigh,
    DWORD dwMoveMethod)
{
    lock_guard<mutex> lock(g_StateMutex);

    auto it = g_FileHandles.find(hFile);
    if (it != g_FileHandles.end() && it->second.isSuspicious) {
        wcout << L"[FAKESUCCESS] SetFilePointer blocked for: " << it->second.filename << endl;

        // Return fake success
        SetLastError(ERROR_SUCCESS);
        return (DWORD)lDistanceToMove;
    }

    return SetFilePointer(hFile, lDistanceToMove, lpDistanceToMoveHigh, dwMoveMethod);
}

// Hook: FlushFileBuffers
BOOL WINAPI myFlushFileBuffersHook(HANDLE hFile)
{
    lock_guard<mutex> lock(g_StateMutex);

    auto it = g_FileHandles.find(hFile);
    if (it != g_FileHandles.end() && it->second.isSuspicious) {
        wcout << L"[FAKESUCCESS] FlushFileBuffers blocked for: " << it->second.filename << endl;

        SetLastError(ERROR_SUCCESS);
        return TRUE;
    }

    return FlushFileBuffers(hFile);
}

// Hook: CloseHandle
BOOL WINAPI myCloseHandleHook(HANDLE hObject)
{
    lock_guard<mutex> lock(g_StateMutex);

    auto it = g_FileHandles.find(hObject);
    if (it != g_FileHandles.end()) {
        if (it->second.isSuspicious) {
            wcout << L"[FAKESUCCESS] CloseHandle for protected file: "
                << it->second.filename
                << L" (Fake bytes written: " << it->second.bytesWritten << L")" << endl;

            g_FileHandles.erase(it);
            SetLastError(ERROR_SUCCESS);
            return TRUE;
        }
        g_FileHandles.erase(it);
    }

    return CloseHandle(hObject);
}

// ============================================================================
// INJECTION ENTRY POINT
// ============================================================================

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
    cout << "[*] ========================================" << endl;
    cout << "[*] Ransomware Defense Hook Initialized" << endl;
    cout << "[*] ========================================" << endl;

    HOOK_TRACE_INFO hCreateFileAHook = { NULL };
    HOOK_TRACE_INFO hCreateFileWHook = { NULL };
    HOOK_TRACE_INFO hWriteFileHook = { NULL };
    HOOK_TRACE_INFO hReadFileHook = { NULL };
    HOOK_TRACE_INFO hSetFilePointerHook = { NULL };
    HOOK_TRACE_INFO hFlushFileBuffersHook = { NULL };
    HOOK_TRACE_INFO hCloseHandleHook = { NULL };
    HOOK_TRACE_INFO hGetFileSizeHook = { NULL };

    // Install hooks
    HMODULE hKernel32 = GetModuleHandle(TEXT("kernel32"));

    LhInstallHook(GetProcAddress(hKernel32, "CreateFileA"),
        myCreateFileAHook, nullptr, &hCreateFileAHook);

    LhInstallHook(GetProcAddress(hKernel32, "CreateFileW"),
        myCreateFileWHook, nullptr, &hCreateFileWHook);

    LhInstallHook(GetProcAddress(hKernel32, "WriteFile"),
        myWriteFileHook, nullptr, &hWriteFileHook);

    LhInstallHook(GetProcAddress(hKernel32, "ReadFile"),
        myReadFileHook, nullptr, &hReadFileHook);

    LhInstallHook(GetProcAddress(hKernel32, "SetFilePointer"),
        mySetFilePointerHook, nullptr, &hSetFilePointerHook);

    LhInstallHook(GetProcAddress(hKernel32, "FlushFileBuffers"),
        myFlushFileBuffersHook, nullptr, &hFlushFileBuffersHook);

    LhInstallHook(GetProcAddress(hKernel32, "CloseHandle"),
        myCloseHandleHook, nullptr, &hCloseHandleHook);

    LhInstallHook(GetProcAddress(hKernel32, "GetFileSize"),
        myGetFileSizeHook, nullptr, &hGetFileSizeHook);

    // Enable all hooks for all threads
    ULONG ACLEntries[1] = { 0 };
    LhSetExclusiveACL(ACLEntries, 1, &hCreateFileAHook);
    LhSetExclusiveACL(ACLEntries, 1, &hCreateFileWHook);
    LhSetExclusiveACL(ACLEntries, 1, &hWriteFileHook);
    LhSetExclusiveACL(ACLEntries, 1, &hReadFileHook);
    LhSetExclusiveACL(ACLEntries, 1, &hSetFilePointerHook);
    LhSetExclusiveACL(ACLEntries, 1, &hFlushFileBuffersHook);
    LhSetExclusiveACL(ACLEntries, 1, &hCloseHandleHook);
    LhSetExclusiveACL(ACLEntries, 1, &hGetFileSizeHook);

    cout << "[+] All hooks installed successfully" << endl;
    cout << "[*] AGGRESSIVE MODE: Blocking ALL write access to protected files" << endl;
    cout << "[*] Monitoring for ransomware behavior..." << endl;
    cout << "[*] Protected extensions: ";
    for (const auto& ext : PROTECTED_EXTENSIONS) {
        wcout << ext << L" ";
    }
    cout << endl;
}