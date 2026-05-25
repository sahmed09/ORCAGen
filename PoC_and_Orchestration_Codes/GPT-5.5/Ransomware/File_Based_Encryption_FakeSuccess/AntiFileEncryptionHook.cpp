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

static unordered_set<HANDLE> gProtectedHandles;
static mutex gHandleLock;

bool EndsWithIgnoreCase(const wstring& value, const wstring& suffix)
{
    if (value.length() < suffix.length())
        return false;

    wstring a = value.substr(value.length() - suffix.length());
    wstring b = suffix;

    transform(a.begin(), a.end(), a.begin(), ::towlower);
    transform(b.begin(), b.end(), b.begin(), ::towlower);

    return a == b;
}

bool IsTargetExtension(const wstring& path)
{
    for (const auto& ext : TARGET_EXTENSIONS)
    {
        if (EndsWithIgnoreCase(path, ext))
            return true;
    }
    return false;
}

bool HasWriteAccess(DWORD access)
{
    return (access & GENERIC_WRITE) ||
        (access & FILE_WRITE_DATA) ||
        (access & FILE_APPEND_DATA) ||
        (access & FILE_WRITE_ATTRIBUTES) ||
        (access & FILE_WRITE_EA);
}

wstring AnsiToWide(LPCSTR input)
{
    if (!input)
        return L"";

    int size = MultiByteToWideChar(CP_ACP, 0, input, -1, nullptr, 0);
    if (size <= 0)
        return L"";

    wstring result(size, L'\0');
    MultiByteToWideChar(CP_ACP, 0, input, -1, &result[0], size);

    if (!result.empty() && result.back() == L'\0')
        result.pop_back();

    return result;
}

void MarkProtectedHandle(HANDLE hFile, const wstring& path)
{
    if (hFile == INVALID_HANDLE_VALUE || hFile == nullptr)
        return;

    lock_guard<mutex> lock(gHandleLock);
    gProtectedHandles.insert(hFile);

    wcout << L"[FakeSuccess] Protected read-only handle: " << path << endl;
}

bool IsProtectedHandle(HANDLE hFile)
{
    lock_guard<mutex> lock(gHandleLock);
    return gProtectedHandles.find(hFile) != gProtectedHandles.end();
}

void RemoveProtectedHandle(HANDLE hFile)
{
    lock_guard<mutex> lock(gHandleLock);
    gProtectedHandles.erase(hFile);
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
    wstring path = AnsiToWide(lpFileName);

    if (IsTargetExtension(path) && HasWriteAccess(dwDesiredAccess))
    {
        cout << "[FakeSuccess] CreateFileA write request downgraded to READ-ONLY: "
            << (lpFileName ? lpFileName : "(null)") << endl;

        HANDLE hFile = CreateFileA(
            lpFileName,
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            lpSecurityAttributes,
            OPEN_EXISTING,
            dwFlagsAndAttributes,
            hTemplateFile
        );

        if (hFile != INVALID_HANDLE_VALUE)
            MarkProtectedHandle(hFile, path);

        SetLastError(ERROR_SUCCESS);
        return hFile;
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

    if (IsTargetExtension(path) && HasWriteAccess(dwDesiredAccess))
    {
        wcout << L"[FakeSuccess] CreateFileW write request downgraded to READ-ONLY: "
            << path << endl;

        HANDLE hFile = CreateFileW(
            lpFileName,
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            lpSecurityAttributes,
            OPEN_EXISTING,
            dwFlagsAndAttributes,
            hTemplateFile
        );

        if (hFile != INVALID_HANDLE_VALUE)
            MarkProtectedHandle(hFile, path);

        SetLastError(ERROR_SUCCESS);
        return hFile;
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
    if (IsProtectedHandle(hFile))
    {
        cout << "[FakeSuccess] Suppressed WriteFile. "
            << "Reported success without modifying the file." << endl;

        if (lpNumberOfBytesWritten)
            *lpNumberOfBytesWritten = nNumberOfBytesToWrite;

        SetLastError(ERROR_SUCCESS);
        return TRUE;
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
    return SetFilePointer(
        hFile,
        lDistanceToMove,
        lpDistanceToMoveHigh,
        dwMoveMethod
    );
}

BOOL WINAPI mySetFilePointerExHook(
    HANDLE hFile,
    LARGE_INTEGER liDistanceToMove,
    PLARGE_INTEGER lpNewFilePointer,
    DWORD dwMoveMethod)
{
    return SetFilePointerEx(
        hFile,
        liDistanceToMove,
        lpNewFilePointer,
        dwMoveMethod
    );
}

BOOL WINAPI myFlushFileBuffersHook(HANDLE hFile)
{
    if (IsProtectedHandle(hFile))
    {
        cout << "[FakeSuccess] FlushFileBuffers faked." << endl;
        SetLastError(ERROR_SUCCESS);
        return TRUE;
    }

    return FlushFileBuffers(hFile);
}

BOOL WINAPI myCloseHandleHook(HANDLE hObject)
{
    if (IsProtectedHandle(hObject))
    {
        cout << "[FakeSuccess] Removing protected handle from tracking." << endl;
        RemoveProtectedHandle(hObject);
    }

    return CloseHandle(hObject);
}

void InstallOneHook(
    HMODULE module,
    LPCSTR functionName,
    void* hookFunction,
    HOOK_TRACE_INFO* hookInfo)
{
    if (!module)
        return;

    FARPROC target = GetProcAddress(module, functionName);
    if (!target)
    {
        cout << "[-] Could not resolve " << functionName << endl;
        return;
    }

    NTSTATUS result = LhInstallHook(
        target,
        hookFunction,
        nullptr,
        hookInfo
    );

    if (FAILED(result))
    {
        wcout << L"[-] Failed to install hook for "
            << functionName << L": "
            << RtlGetLastErrorString() << endl;
        return;
    }

    ULONG ACLEntries[1] = { 0 };
    LhSetExclusiveACL(ACLEntries, 1, hookInfo);

    cout << "[+] Hook installed: " << functionName << endl;
}

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(
    REMOTE_ENTRY_INFO * inRemoteInfo)
{
    cout << "[*] Anti-ransomware FakeSuccess DLL injected." << endl;

    HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
    HMODULE kernelBase = GetModuleHandleW(L"KernelBase.dll");

    static HOOK_TRACE_INFO hCreateFileA_K32 = { NULL };
    static HOOK_TRACE_INFO hCreateFileW_K32 = { NULL };
    static HOOK_TRACE_INFO hWriteFile_K32 = { NULL };
    static HOOK_TRACE_INFO hSetFilePointer_K32 = { NULL };
    static HOOK_TRACE_INFO hSetFilePointerEx_K32 = { NULL };
    static HOOK_TRACE_INFO hFlushFileBuffers_K32 = { NULL };
    static HOOK_TRACE_INFO hCloseHandle_K32 = { NULL };

    static HOOK_TRACE_INFO hCreateFileA_KB = { NULL };
    static HOOK_TRACE_INFO hCreateFileW_KB = { NULL };
    static HOOK_TRACE_INFO hWriteFile_KB = { NULL };
    static HOOK_TRACE_INFO hSetFilePointer_KB = { NULL };
    static HOOK_TRACE_INFO hSetFilePointerEx_KB = { NULL };
    static HOOK_TRACE_INFO hFlushFileBuffers_KB = { NULL };
    static HOOK_TRACE_INFO hCloseHandle_KB = { NULL };

    InstallOneHook(kernel32, "CreateFileA", myCreateFileAHook, &hCreateFileA_K32);
    InstallOneHook(kernel32, "CreateFileW", myCreateFileWHook, &hCreateFileW_K32);
    InstallOneHook(kernel32, "WriteFile", myWriteFileHook, &hWriteFile_K32);
    InstallOneHook(kernel32, "SetFilePointer", mySetFilePointerHook, &hSetFilePointer_K32);
    InstallOneHook(kernel32, "SetFilePointerEx", mySetFilePointerExHook, &hSetFilePointerEx_K32);
    InstallOneHook(kernel32, "FlushFileBuffers", myFlushFileBuffersHook, &hFlushFileBuffers_K32);
    InstallOneHook(kernel32, "CloseHandle", myCloseHandleHook, &hCloseHandle_K32);

    InstallOneHook(kernelBase, "CreateFileA", myCreateFileAHook, &hCreateFileA_KB);
    InstallOneHook(kernelBase, "CreateFileW", myCreateFileWHook, &hCreateFileW_KB);
    InstallOneHook(kernelBase, "WriteFile", myWriteFileHook, &hWriteFile_KB);
    InstallOneHook(kernelBase, "SetFilePointer", mySetFilePointerHook, &hSetFilePointer_KB);
    InstallOneHook(kernelBase, "SetFilePointerEx", mySetFilePointerExHook, &hSetFilePointerEx_KB);
    InstallOneHook(kernelBase, "FlushFileBuffers", myFlushFileBuffersHook, &hFlushFileBuffers_KB);
    InstallOneHook(kernelBase, "CloseHandle", myCloseHandleHook, &hCloseHandle_KB);

    cout << "[+] FakeSuccess hooks installed successfully." << endl;
}