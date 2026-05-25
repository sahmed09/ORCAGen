#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <easyhook.h>

#pragma comment(lib, "EasyHook32.lib")

using namespace std;

// Global flag to track if we should block file enumeration
DWORD gFreqOffset = 0;

// Original function pointers (stored for calling real implementations if needed)
typedef HANDLE(WINAPI* FindFirstFileA_t)(LPCSTR, LPWIN32_FIND_DATAA);
typedef BOOL(WINAPI* FindNextFileA_t)(HANDLE, LPWIN32_FIND_DATAA);

FindFirstFileA_t pOriginalFindFirstFileA = nullptr;
FindNextFileA_t pOriginalFindNextFileA = nullptr;

// Hook for FindFirstFileA - returns fake failure
HANDLE WINAPI myFindFirstFileAHook(
    LPCSTR lpFileName,
    LPWIN32_FIND_DATAA lpFindFileData)
{
    cout << "[+] FindFirstFileA Hook Triggered!" << endl;
    cout << "[*] Intercepted search path: " << lpFileName << endl;
    cout << "[!] Returning fake failure to protect sensitive files" << endl;

    // Set error code to simulate access denied
    SetLastError(ERROR_ACCESS_DENIED);

    // Return invalid handle to indicate failure
    return INVALID_HANDLE_VALUE;
}

// Hook for FindNextFileA - returns fake failure
BOOL WINAPI myFindNextFileAHook(
    HANDLE hFindFile,
    LPWIN32_FIND_DATAA lpFindFileData)
{
    cout << "[+] FindNextFileA Hook Triggered!" << endl;
    cout << "[!] Returning fake failure to interrupt file enumeration" << endl;

    // Set error code to simulate no more files
    SetLastError(ERROR_NO_MORE_FILES);

    // Return FALSE to indicate failure/end of enumeration
    return FALSE;
}

// Beep hook (kept from original for testing)
BOOL WINAPI myBeepHook(DWORD dwFreq, DWORD dwDuration)
{
    cout << "[+] BeepHook triggered!" << endl;
    cout << "Original Frequency: " << dwFreq << ", Duration: " << dwDuration << endl;
    return Beep(dwFreq + gFreqOffset, dwDuration);
}

// DLL Entry Point for EasyHook injection
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
    cout << "[*] Injection started - Anti-File-Scanner Defense Active" << endl;

    // Check if user data was passed
    if (inRemoteInfo->UserDataSize == sizeof(DWORD))
    {
        gFreqOffset = *reinterpret_cast<DWORD*>(inRemoteInfo->UserData);
        cout << "[*] Frequency Offset Received: " << gFreqOffset << endl;
    }

    // Hook trace information structures
    HOOK_TRACE_INFO hFindFirstFileHook = { NULL };
    HOOK_TRACE_INFO hFindNextFileHook = { NULL };
    HOOK_TRACE_INFO hBeepHook = { NULL };

    // Get function addresses from kernel32.dll
    FARPROC findFirstFileAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "FindFirstFileA");
    FARPROC findNextFileAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "FindNextFileA");
    FARPROC beepAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "Beep");

    cout << "[*] Function addresses:" << endl;
    cout << "  - FindFirstFileA: " << (void*)findFirstFileAddr << endl;
    cout << "  - FindNextFileA: " << (void*)findNextFileAddr << endl;
    cout << "  - Beep: " << (void*)beepAddr << endl;

    // Install hooks
    NTSTATUS result1 = LhInstallHook(findFirstFileAddr, myFindFirstFileAHook, nullptr, &hFindFirstFileHook);
    NTSTATUS result2 = LhInstallHook(findNextFileAddr, myFindNextFileAHook, nullptr, &hFindNextFileHook);
    NTSTATUS result3 = LhInstallHook(beepAddr, myBeepHook, nullptr, &hBeepHook);

    // Check installation results
    if (FAILED(result1))
    {
        wstring s(RtlGetLastErrorString());
        wcerr << L"[!] Failed to install FindFirstFileA hook: " << s << endl;
    }
    else
    {
        cout << "[+] FindFirstFileA hook installed successfully!" << endl;
    }

    if (FAILED(result2))
    {
        wstring s(RtlGetLastErrorString());
        wcerr << L"[!] Failed to install FindNextFileA hook: " << s << endl;
    }
    else
    {
        cout << "[+] FindNextFileA hook installed successfully!" << endl;
    }

    if (FAILED(result3))
    {
        wstring s(RtlGetLastErrorString());
        wcerr << L"[!] Failed to install Beep hook: " << s << endl;
    }
    else
    {
        cout << "[+] Beep hook installed successfully!" << endl;
    }

    // Set ACL to apply hooks to all threads (thread ID 0 means all threads)
    ULONG ACLEntries[1] = { 0 };
    LhSetExclusiveACL(ACLEntries, 1, &hFindFirstFileHook);
    LhSetExclusiveACL(ACLEntries, 1, &hFindNextFileHook);
    LhSetExclusiveACL(ACLEntries, 1, &hBeepHook);

    cout << "[*] All hooks enabled. File scanning operations will be blocked." << endl;
}