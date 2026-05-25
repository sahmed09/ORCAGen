#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <easyhook.h>

#include <iostream>
#include <string>
#include <vector>
#include <unordered_set>
#include <mutex>
#include <algorithm>

#pragma comment(lib, "EasyHook32.lib") // use EasyHook64.lib for x64

using namespace std;

static const vector<wstring> TARGET_EXTENSIONS = {
    L".docx", L".xlsx", L".db", L".txt"
};

static unordered_set<HANDLE> gBlockedHandles;
static mutex gLock;

static bool EndsWithIgnoreCase(const wstring& value, const wstring& suffix)
{
    if (value.length() < suffix.length())
        return false;

    wstring tail = value.substr(value.length() - suffix.length());

    transform(tail.begin(), tail.end(), tail.begin(), ::towlower);

    wstring loweredSuffix = suffix;
    transform(loweredSuffix.begin(), loweredSuffix.end(), loweredSuffix.begin(), ::towlower);

    return tail == loweredSuffix;
}

static bool IsTargetFileType(const wstring& path)
{
    for (const auto& ext : TARGET_EXTENSIONS)
    {
        if (EndsWithIgnoreCase(path, ext))
            return true;
    }
    return false;
}

static bool HasSuspiciousWriteAccess(DWORD desiredAccess)
{
    return (desiredAccess & GENERIC_WRITE) ||
        (desiredAccess & FILE_WRITE_DATA) ||
        (desiredAccess & FILE_APPEND_DATA) ||
        (desiredAccess & FILE_WRITE_ATTRIBUTES) ||
        (desiredAccess & FILE_WRITE_EA);
}

static wstring AnsiToWideSafe(LPCSTR str)
{
    if (!str) return L"";

    int size = MultiByteToWideChar(CP_ACP, 0, str, -1, nullptr, 0);
    if (size <= 0) return L"";

    wstring result(size, L'\0');
    MultiByteToWideChar(CP_ACP, 0, str, -1, &result[0], size);

    if (!result.empty() && result.back() == L'\0')
        result.pop_back();

    return result;
}

HANDLE WINAPI myCreateFileAHook(
    LPCSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile)
{
    wstring path = AnsiToWideSafe(lpFileName);

    if (IsTargetFileType(path) && HasSuspiciousWriteAccess(dwDesiredAccess))
    {
        cout << "[FakeFailure] Blocked CreateFileA encryption attempt: ";
        cout << (lpFileName ? lpFileName : "(null)") << endl;

        SetLastError(ERROR_ACCESS_DENIED);
        return INVALID_HANDLE_VALUE;
    }

    return CreateFileA(
        lpFileName,
        dwDesiredAccess,
        dwShareMode,
        lpSecurityAttributes,
        dwCreationDisposition,
        dwFlagsAndAttributes,
        hTemplateFile
    );
}

HANDLE WINAPI myCreateFileWHook(
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile)
{
    wstring path = lpFileName ? lpFileName : L"";

    if (IsTargetFileType(path) && HasSuspiciousWriteAccess(dwDesiredAccess))
    {
        wcout << L"[FakeFailure] Blocked CreateFileW encryption attempt: "
            << path << endl;

        SetLastError(ERROR_ACCESS_DENIED);
        return INVALID_HANDLE_VALUE;
    }

    return CreateFileW(
        lpFileName,
        dwDesiredAccess,
        dwShareMode,
        lpSecurityAttributes,
        dwCreationDisposition,
        dwFlagsAndAttributes,
        hTemplateFile
    );
}

BOOL WINAPI myWriteFileHook(
    HANDLE hFile,
    LPCVOID lpBuffer,
    DWORD nNumberOfBytesToWrite,
    LPDWORD lpNumberOfBytesWritten,
    LPOVERLAPPED lpOverlapped)
{
    {
        lock_guard<mutex> guard(gLock);
        if (gBlockedHandles.find(hFile) != gBlockedHandles.end())
        {
            cout << "[FakeFailure] Blocked WriteFile on protected handle." << endl;

            if (lpNumberOfBytesWritten)
                *lpNumberOfBytesWritten = 0;

            SetLastError(ERROR_WRITE_PROTECT);
            return FALSE;
        }
    }

    return WriteFile(
        hFile,
        lpBuffer,
        nNumberOfBytesToWrite,
        lpNumberOfBytesWritten,
        lpOverlapped
    );
}

DWORD WINAPI mySetFilePointerHook(
    HANDLE hFile,
    LONG lDistanceToMove,
    PLONG lpDistanceToMoveHigh,
    DWORD dwMoveMethod)
{
    {
        lock_guard<mutex> guard(gLock);
        if (gBlockedHandles.find(hFile) != gBlockedHandles.end())
        {
            cout << "[FakeFailure] Blocked SetFilePointer on protected handle." << endl;
            SetLastError(ERROR_ACCESS_DENIED);
            return INVALID_SET_FILE_POINTER;
        }
    }

    return SetFilePointer(
        hFile,
        lDistanceToMove,
        lpDistanceToMoveHigh,
        dwMoveMethod
    );
}

BOOL WINAPI myFlushFileBuffersHook(HANDLE hFile)
{
    {
        lock_guard<mutex> guard(gLock);
        if (gBlockedHandles.find(hFile) != gBlockedHandles.end())
        {
            cout << "[FakeFailure] Blocked FlushFileBuffers on protected handle." << endl;
            SetLastError(ERROR_ACCESS_DENIED);
            return FALSE;
        }
    }

    return FlushFileBuffers(hFile);
}

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(
    REMOTE_ENTRY_INFO * inRemoteInfo)
{
    cout << "[*] Anti-ransomware FakeFailure hook injected." << endl;

    HOOK_TRACE_INFO hCreateFileA = { NULL };
    HOOK_TRACE_INFO hCreateFileW = { NULL };
    HOOK_TRACE_INFO hWriteFile = { NULL };
    HOOK_TRACE_INFO hSetFilePointer = { NULL };
    HOOK_TRACE_INFO hFlushFileBuffers = { NULL };

    HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");

    LhInstallHook(
        GetProcAddress(kernel32, "CreateFileA"),
        myCreateFileAHook,
        nullptr,
        &hCreateFileA
    );

    LhInstallHook(
        GetProcAddress(kernel32, "CreateFileW"),
        myCreateFileWHook,
        nullptr,
        &hCreateFileW
    );

    LhInstallHook(
        GetProcAddress(kernel32, "WriteFile"),
        myWriteFileHook,
        nullptr,
        &hWriteFile
    );

    LhInstallHook(
        GetProcAddress(kernel32, "SetFilePointer"),
        mySetFilePointerHook,
        nullptr,
        &hSetFilePointer
    );

    LhInstallHook(
        GetProcAddress(kernel32, "FlushFileBuffers"),
        myFlushFileBuffersHook,
        nullptr,
        &hFlushFileBuffers
    );

    ULONG ACLEntries[1] = { 0 };

    LhSetExclusiveACL(ACLEntries, 1, &hCreateFileA);
    LhSetExclusiveACL(ACLEntries, 1, &hCreateFileW);
    LhSetExclusiveACL(ACLEntries, 1, &hWriteFile);
    LhSetExclusiveACL(ACLEntries, 1, &hSetFilePointer);
    LhSetExclusiveACL(ACLEntries, 1, &hFlushFileBuffers);

    cout << "[+] Hooks installed successfully." << endl;
}