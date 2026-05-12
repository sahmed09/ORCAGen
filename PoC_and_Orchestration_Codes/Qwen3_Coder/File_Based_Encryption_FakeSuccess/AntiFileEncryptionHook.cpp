// RansomwareDefense.cpp
#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <set>
#include <unordered_map>
#include <Psapi.h>
#include <easyhook.h>

#pragma comment(lib, "EasyHook32.lib")

using namespace std;

const vector<string> TARGET_EXTENSIONS = {
    ".txt", ".docx", ".xlsx", ".db", ".pdf", ".jpg", ".png", ".gif",
    ".mp4", ".avi", ".mov", ".mp3", ".wav"
};

// Track file handles being fake-encrypted
unordered_map<HANDLE, wstring> fakeEncryptedHandles;

bool IsTargetFile(const wstring& path) {
    size_t dot = path.find_last_of(L'.');
    if (dot == wstring::npos) return false;
    wstring ext = path.substr(dot);
    transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
    for (auto& t : TARGET_EXTENSIONS) {
        wstring we(t.begin(), t.end());
        if (ext == we) return true;
    }
    return false;
}

wstring GetFileNameFromHandle(HANDLE hFile) {
    WCHAR path[MAX_PATH];
    if (GetFinalPathNameByHandleW(hFile, path, MAX_PATH, VOLUME_NAME_NT)) {
        return wstring(path);
    }
    return L"";
}

// ---------------- HOOKED FUNCTIONS ----------------

HANDLE(WINAPI* TrueCreateFileW)(
    LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE
    ) = CreateFileW;

HANDLE WINAPI myCreateFileW(
    LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
    HANDLE hFile = TrueCreateFileW(lpFileName, dwDesiredAccess, dwShareMode,
        lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);

    if (hFile != INVALID_HANDLE_VALUE && IsTargetFile(lpFileName)) {
        fakeEncryptedHandles[hFile] = lpFileName;
        wcout << L"[Deception] FakeEncrypt tracking started for: " << lpFileName << endl;
    }

    return hFile;
}

BOOL(WINAPI* TrueWriteFile)(
    HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED
    ) = WriteFile;

BOOL WINAPI myWriteFile(
    HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
    LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped)
{
    if (fakeEncryptedHandles.count(hFile)) {
        if (lpNumberOfBytesWritten) *lpNumberOfBytesWritten = nNumberOfBytesToWrite;
        wcout << L"[Deception] WriteFile intercepted and neutralized for: "
            << fakeEncryptedHandles[hFile] << endl;
        return TRUE; // Fake success
    }

    return TrueWriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
}

DWORD(WINAPI* TrueSetFilePointer)(
    HANDLE, LONG, PLONG, DWORD) = SetFilePointer;

DWORD WINAPI mySetFilePointer(
    HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod)
{
    if (fakeEncryptedHandles.count(hFile)) {
        wcout << L"[Deception] SetFilePointer intercepted for: " << fakeEncryptedHandles[hFile] << endl;
        return 0; // Fake success
    }

    return TrueSetFilePointer(hFile, lDistanceToMove, lpDistanceToMoveHigh, dwMoveMethod);
}

BOOL(WINAPI* TrueFlushFileBuffers)(HANDLE) = FlushFileBuffers;

BOOL WINAPI myFlushFileBuffers(HANDLE hFile) {
    if (fakeEncryptedHandles.count(hFile)) {
        wcout << L"[Deception] FlushFileBuffers intercepted for: " << fakeEncryptedHandles[hFile] << endl;
        return TRUE; // Pretend buffer is flushed
    }

    return TrueFlushFileBuffers(hFile);
}

BOOL(WINAPI* TrueCloseHandle)(HANDLE) = CloseHandle;

BOOL WINAPI myCloseHandle(HANDLE hObject) {
    fakeEncryptedHandles.erase(hObject); // Cleanup
    return TrueCloseHandle(hObject);
}

// ---------------- HOOK ENTRY ----------------

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo) {
    HOOK_TRACE_INFO h1 = {}, h2 = {}, h3 = {}, h4 = {}, h5 = {};

    LhInstallHook(GetProcAddress(GetModuleHandleW(L"kernel32"), "CreateFileW"),
        myCreateFileW, NULL, &h1);
    LhInstallHook(GetProcAddress(GetModuleHandleW(L"kernel32"), "WriteFile"),
        myWriteFile, NULL, &h2);
    LhInstallHook(GetProcAddress(GetModuleHandleW(L"kernel32"), "SetFilePointer"),
        mySetFilePointer, NULL, &h3);
    LhInstallHook(GetProcAddress(GetModuleHandleW(L"kernel32"), "FlushFileBuffers"),
        myFlushFileBuffers, NULL, &h4);
    LhInstallHook(GetProcAddress(GetModuleHandleW(L"kernel32"), "CloseHandle"),
        myCloseHandle, NULL, &h5);

    ULONG ACLEntries[1] = { 0 };
    LhSetExclusiveACL(ACLEntries, 1, &h1);
    LhSetExclusiveACL(ACLEntries, 1, &h2);
    LhSetExclusiveACL(ACLEntries, 1, &h3);
    LhSetExclusiveACL(ACLEntries, 1, &h4);
    LhSetExclusiveACL(ACLEntries, 1, &h5);

    wcout << L"[*] FakeSuccess ransomware defense hooks installed." << endl;
}
