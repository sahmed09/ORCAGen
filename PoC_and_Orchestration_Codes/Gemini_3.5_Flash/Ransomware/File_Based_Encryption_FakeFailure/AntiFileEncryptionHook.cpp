#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlwapi.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <easyhook.h>

#pragma comment(lib, "EasyHook32.lib") // Match target architecture (EasyHook32.lib / EasyHook64.lib)
#pragma comment(lib, "Shlwapi.lib")

using namespace std;

// List of protected extensions monitored by the deception engine
const std::vector<std::wstring> PROTECTED_EXTENSIONS = { L".docx", L".xlsx", L".db", L".txt", L".png" };

// Function pointers to call original Windows APIs
typedef HANDLE(WINAPI* CreateFileW_t)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
typedef BOOL(WINAPI* WriteFile_t)(HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED);

CreateFileW_t fpCreateFileW = nullptr;
WriteFile_t fpWriteFile = nullptr;

// Helper function to check if a file path matches the deception criteria
bool IsTargetForDeception(LPCWSTR lpFileName, DWORD dwDesiredAccess)
{
    if (lpFileName == nullptr) return false;

    // 1. Check if the operation intent involves writing/modifying data
    bool isWriteOperation = (dwDesiredAccess & GENERIC_WRITE) ||
        (dwDesiredAccess & FILE_WRITE_DATA) ||
        (dwDesiredAccess & GENERIC_ALL);

    if (!isWriteOperation) return false;

    // 2. Extract and parse extension
    LPCWSTR ext = PathFindExtensionW(lpFileName);
    if (ext && *ext)
    {
        std::wstring extStr(ext);
        // Normalize to lowercase for robust matching
        std::transform(extStr.begin(), extStr.end(), extStr.begin(), ::tolower);

        for (const auto& protectedExt : PROTECTED_EXTENSIONS)
        {
            if (extStr == protectedExt)
            {
                return true;
            }
        }
    }
    return false;
}

// Intercepted CreateFileW Hook
HANDLE WINAPI myCreateFileWHook(
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile)
{
    if (IsTargetForDeception(lpFileName, dwDesiredAccess))
    {
        wcout << L"[Deception Engine] Intercepted unauthorized modification attempt on: " << lpFileName << endl;

        // Return a FakeFailure state to the calling process
        SetLastError(ERROR_ACCESS_DENIED);
        return INVALID_HANDLE_VALUE;
    }

    // Pass through for legitimate files or read-only requests
    return CreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
        dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

// Intercepted WriteFile Hook (Secondary defense tier)
BOOL WINAPI myWriteFileHook(
    HANDLE hFile,
    LPCVOID lpBuffer,
    DWORD nNumberOfBytesToWrite,
    LPDWORD lpNumberOfBytesWritten,
    LPOVERLAPPED lpOverlapped)
{
    wchar_t filePath[MAX_PATH] = { 0 };

    // Resolve handle to path to confirm context if handle was opened before hook installation
    if (GetFinalPathNameByHandleW(hFile, filePath, MAX_PATH, FILE_NAME_NORMALIZED) > 0)
    {
        if (IsTargetForDeception(filePath, GENERIC_WRITE))
        {
            cout << "[Deception Engine] Blocked WriteFile execution on protected resource." << endl;
            if (lpNumberOfBytesWritten) *lpNumberOfBytesWritten = 0;
            SetLastError(ERROR_ACCESS_DENIED);
            return FALSE;
        }
    }

    return WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
}

// EasyHook Entrance Injection Interface
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
    cout << "[*] Active Defense Deception Engine Injected." << endl;

    HOOK_TRACE_INFO hCreateFileHook = { NULL };
    HOOK_TRACE_INFO hWriteFileHook = { NULL };

    // Resolve system module targets
    HMODULE hKernel32 = GetModuleHandle(TEXT("kernel32"));
    FARPROC createFileAddr = GetProcAddress(hKernel32, "CreateFileW");
    FARPROC writeFileAddr = GetProcAddress(hKernel32, "WriteFile");

    // Install Runtime Hooks
    NTSTATUS statusCreate = LhInstallHook(createFileAddr, myCreateFileWHook, nullptr, &hCreateFileHook);
    NTSTATUS statusWrite = LhInstallHook(writeFileAddr, myWriteFileHook, nullptr, &hWriteFileHook);

    if (FAILED(statusCreate) || FAILED(statusWrite))
    {
        cout << "[-] Error initializing active defense hooks." << endl;
        return;
    }

    // Activate interception globally inside target runtime context
    ULONG ACLEntries[1] = { 0 };
    LhSetExclusiveACL(ACLEntries, 1, &hCreateFileHook);
    LhSetExclusiveACL(ACLEntries, 1, &hWriteFileHook);

    cout << "[+] Dynamic FakeFailure protection profiles deployed successfully." << endl;
}