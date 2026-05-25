#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <cstring>
#include <easyhook.h>

#pragma comment(lib, "EasyHook32.lib") // Adjust to EasyHook64.lib if compiling for x64

using namespace std;

// Function pointers for invoking the original Windows APIs
typedef HANDLE(WINAPI* FindFirstFileA_t)(LPCSTR lpFileName, LPWIN32_FIND_DATAA lpFindFileData);
typedef BOOL(WINAPI* FindNextFileA_t)(HANDLE hFindFile, LPWIN32_FIND_DATAA lpFindFileData);

FindFirstFileA_t OriginalFindFirstFileA = nullptr;
FindNextFileA_t OriginalFindNextFileA = nullptr;

// Helper function to apply the deception layer to the find data structure
void ApplyFileDeception(LPWIN32_FIND_DATAA lpFindFileData)
{
    if (lpFindFileData == nullptr) return;

    string currentName = lpFindFileData->cFileName;

    // Do not alter relative directory navigation links
    if (currentName == "." || currentName == "..")
    {
        return;
    }

    // Check if the current item found is a regular file (not a directory)
    if (!(lpFindFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
        size_t dotPos = currentName.find_last_of('.');
        if (dotPos != string::npos)
        {
            string ext = currentName.substr(dotPos);

            // If the file matches any targeted configuration extensions, mask it
            if (ext == ".txt" || ext == ".json" || ext == ".conf" || ext == ".ini")
            {
                cout << "[Deception] Intercepted real file: " << currentName << "\n";

                // Supply a safe decoy name instead of the real filename
                string decoyName = "decoy_configuration" + ext;

                // Ensure we do not overflow the fixed-size Win32 buffer (MAX_PATH)
                memset(lpFindFileData->cFileName, 0, sizeof(lpFindFileData->cFileName));
                strncpy_s(lpFindFileData->cFileName, decoyName.c_str(), _TRUNCATE);

                cout << "[Deception] Substituted with decoy: " << lpFindFileData->cFileName << "\n";
            }
        }
    }
}

// Hook function for FindFirstFileA
HANDLE WINAPI myFindFirstFileAHook(LPCSTR lpFileName, LPWIN32_FIND_DATAA lpFindFileData)
{
    // Call the original function to populate the initial buffer safely
    HANDLE hFind = OriginalFindFirstFileA(lpFileName, lpFindFileData);

    if (hFind != INVALID_HANDLE_VALUE)
    {
        // Modify the results before the calling application sees them
        ApplyFileDeception(lpFindFileData);
    }

    return hFind;
}

// Hook function for FindNextFileA
BOOL WINAPI myFindNextFileAHook(HANDLE hFindFile, LPWIN32_FIND_DATAA lpFindFileData)
{
    // Call the original function to retrieve the next file in sequence
    BOOL result = OriginalFindNextFileA(hFindFile, lpFindFileData);

    if (result)
    {
        // Modify the results before the calling application processes them
        ApplyFileDeception(lpFindFileData);
    }

    return result;
}

// Entry point called by EasyHook injector framework
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
    cout << "[*] Cyber Deception Module Loaded into Target Process.\n";

    HOOK_TRACE_INFO hFindFirstHook = { NULL };
    HOOK_TRACE_INFO hFindNextHook = { NULL };

    HMODULE hKernel32 = GetModuleHandle(TEXT("kernel32"));
    if (hKernel32 == nullptr)
    {
        cerr << "[-] Failed to secure handle for kernel32.dll\n";
        return;
    }

    // Resolve target API addresses
    FARPROC findFirstAddr = GetProcAddress(hKernel32, "FindFirstFileA");
    FARPROC findNextAddr = GetProcAddress(hKernel32, "FindNextFileA");

    // Store original addresses to allow trampoline execution
    OriginalFindFirstFileA = reinterpret_cast<FindFirstFileA_t>(findFirstAddr);
    OriginalFindNextFileA = reinterpret_cast<FindNextFileA_t>(findNextAddr);

    // Install the hooks using EasyHook runtime libraries
    NTSTATUS statusFirst = LhInstallHook(findFirstAddr, myFindFirstFileAHook, nullptr, &hFindFirstHook);
    NTSTATUS statusNext = LhInstallHook(findNextAddr, myFindNextFileAHook, nullptr, &hFindNextHook);

    if (FAILED(statusFirst) || FAILED(statusNext))
    {
        cerr << "[-] Failed to establish hooks within the address space.\n";
        return;
    }

    // Activate the hooks globally within this specific process context
    ULONG ACLEntries[1] = { 0 };
    LhSetExclusiveACL(ACLEntries, 1, &hFindFirstHook);
    LhSetExclusiveACL(ACLEntries, 1, &hFindNextHook);

    cout << "[+] Hooking implementation active. Monitoring file scan operations.\n";
}