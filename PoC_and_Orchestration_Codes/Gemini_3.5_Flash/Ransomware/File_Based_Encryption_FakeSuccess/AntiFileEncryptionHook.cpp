#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_set>
#include <shared_mutex>
#include <algorithm>
#include <easyhook.h>

#pragma comment(lib, "EasyHook32.lib") // Use EasyHook64.lib for x64 target configurations

// Thread-safe container to track handles identified as targeted files
std::unordered_set<HANDLE> g_TargetedHandles;
std::shared_mutex g_HandleMutex;

// Structure to preserve original function pointers if needed for direct calling
typedef HANDLE(WINAPI* CreateFileW_t)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
typedef BOOL(WINAPI* WriteFile_t)(HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED);
typedef BOOL(WINAPI* CloseHandle_t)(HANDLE);

// Helper function to check if a file extension matches our protection list
bool IsProtectedExtension(const std::wstring& filePath) {
    size_t dotPos = filePath.rfind(L'.');
    if (dotPos == std::wstring::npos) return false;

    std::wstring ext = filePath.substr(dotPos);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    // Synchronized with the extensions targeted by the PoC
    static const std::unordered_set<std::wstring> protectedExtensions = {
        L".docx", L".xlsx", L".db", L".txt", L".pdf", L".jpg"
    };

    return protectedExtensions.find(ext) != protectedExtensions.end();
}

// Hooked CreateFileW
HANDLE WINAPI HookedCreateFileW(
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile)
{
    // Check if the file is opened for modification/writing and matches target extensions
    bool isWriteAccess = (dwDesiredAccess & GENERIC_WRITE) || (dwDesiredAccess & FILE_WRITE_DATA);

    if (lpFileName && isWriteAccess && IsProtectedExtension(lpFileName)) {
        std::wcout << L"[Deception] Intercepted potentially malicious write access to: " << lpFileName << std::endl;

        // Open the file with READ access only, stripping WRITE access to protect the file from modification
        DWORD safeAccess = dwDesiredAccess & ~GENERIC_WRITE;
        safeAccess &= ~FILE_WRITE_DATA;
        safeAccess |= GENERIC_READ; // Ensure we can still satisfy read requests if needed

        HANDLE hFile = CreateFileW(lpFileName, safeAccess, dwShareMode, lpSecurityAttributes,
            dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);

        if (hFile != INVALID_HANDLE_VALUE) {
            std::unique_lock<std::shared_mutex> lock(g_HandleMutex);
            g_TargetedHandles.insert(hFile);
        }
        return hFile;
    }

    // Pass through normally for legitimate applications or non-targeted files
    return CreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
        dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

// Hooked WriteFile
BOOL WINAPI HookedWriteFile(
    HANDLE hFile,
    LPCVOID lpBuffer,
    DWORD nNumberOfBytesToWrite,
    LPDWORD lpNumberOfBytesWritten,
    LPOVERLAPPED lpOverlapped)
{
    bool isTargeted = false;
    {
        std::shared_lock<std::shared_mutex> lock(g_HandleMutex);
        isTargeted = (g_TargetedHandles.find(hFile) != g_TargetedHandles.end());
    }

    if (isTargeted) {
        // Implement the FakeSuccess Approach:
        // Do NOT call the real WriteFile. Instead, report that all bytes were written successfully.
        if (lpNumberOfBytesWritten) {
            *lpNumberOfBytesWritten = nNumberOfBytesToWrite;
        }

        std::cout << "[Deception] Blocked encryption write attempt. Returning Fake Success." << std::endl;
        return TRUE;
    }

    // Pass through normally for standard files
    return WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
}

// Hooked CloseHandle
BOOL WINAPI HookedCloseHandle(HANDLE hObject) {
    {
        std::unique_lock<std::shared_mutex> lock(g_HandleMutex);
        g_TargetedHandles.erase(hObject);
    }
    return CloseHandle(hObject);
}

// Injection Entry Point required by EasyHook
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
    std::cout << "\n[*] Cyber Deception Framework: Active Defense Module Active.\n";

    HOOK_TRACE_INFO hCreateFile = { NULL };
    HOOK_TRACE_INFO hWriteFile = { NULL };
    HOOK_TRACE_INFO hCloseHandle = { NULL };

    // Resolve API addresses from kernel32.dll
    HMODULE hKernel32 = GetModuleHandle(TEXT("kernel32"));
    FARPROC createFileAddr = GetProcAddress(hKernel32, "CreateFileW");
    FARPROC writeFileAddr = GetProcAddress(hKernel32, "WriteFile");
    FARPROC closeHandleAddr = GetProcAddress(hKernel32, "CloseHandle");

    // Install runtime hooks
    LhInstallHook(createFileAddr, HookedCreateFileW, nullptr, &hCreateFile);
    LhInstallHook(writeFileAddr, HookedWriteFile, nullptr, &hWriteFile);
    LhInstallHook(closeHandleAddr, HookedCloseHandle, nullptr, &hCloseHandle);

    // Configure Access Control Lists to activate hooks across all threads in the target process
    ULONG ACLEntries[1] = { 0 };
    LhSetExclusiveACL(ACLEntries, 1, &hCreateFile);
    LhSetExclusiveACL(ACLEntries, 1, &hWriteFile);
    LhSetExclusiveACL(ACLEntries, 1, &hCloseHandle);

    std::cout << "[+] Active defense file hooks deployed successfully.\n\n";
}