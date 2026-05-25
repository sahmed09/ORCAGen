#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <easyhook.h>

#pragma comment(lib, "EasyHook32.lib") // Use EasyHook64.lib for x64

using namespace std;

static const vector<wstring> SuspiciousExtensions = {
    L".locked",
    L".enc",
    L".encrypted",
    L".revil"
};

wstring ToLowerW(wstring s)
{
    transform(s.begin(), s.end(), s.begin(), towlower);
    return s;
}

bool EndsWithIgnoreCase(const wstring& value, const wstring& suffix)
{
    if (suffix.size() > value.size())
        return false;

    wstring v = ToLowerW(value);
    wstring s = ToLowerW(suffix);

    return v.compare(v.size() - s.size(), s.size(), s) == 0;
}

bool ContainsSuspiciousExtensionPattern(const wstring& path)
{
    wstring lowerPath = ToLowerW(path);

    for (const auto& ext : SuspiciousExtensions)
    {
        if (EndsWithIgnoreCase(lowerPath, ext))
            return true;
    }

    return false;
}

void LogRenameAttempt(
    const wstring& apiName,
    LPCWSTR oldPath,
    LPCWSTR newPath,
    DWORD flags,
    bool blocked)
{
    wofstream logFile(L"ransomware_rename_hook_log.txt", ios::app);

    if (!logFile.is_open())
        return;

    SYSTEMTIME st;
    GetLocalTime(&st);

    logFile << L"[" << st.wYear << L"-" << st.wMonth << L"-" << st.wDay
        << L" " << st.wHour << L":" << st.wMinute << L":" << st.wSecond
        << L"]\n";

    logFile << L"API: " << apiName << L"\n";
    logFile << L"Old Path: " << (oldPath ? oldPath : L"(null)") << L"\n";
    logFile << L"New Path: " << (newPath ? newPath : L"(null)") << L"\n";
    logFile << L"Flags: " << flags << L"\n";
    logFile << L"Decision: " << (blocked ? L"BLOCKED" : L"ALLOWED") << L"\n";
    logFile << L"----------------------------------------\n";

    logFile.close();
}

BOOL WINAPI myMoveFileWHook(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName)
{
    wcout << L"[Hook] MoveFileW intercepted\n";
    wcout << L"  From: " << (lpExistingFileName ? lpExistingFileName : L"(null)") << endl;
    wcout << L"  To:   " << (lpNewFileName ? lpNewFileName : L"(null)") << endl;

    if (lpNewFileName && ContainsSuspiciousExtensionPattern(lpNewFileName))
    {
        wcout << L"[Defense] Suspicious ransomware rename blocked." << endl;

        LogRenameAttempt(
            L"MoveFileW",
            lpExistingFileName,
            lpNewFileName,
            0,
            true
        );

        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    LogRenameAttempt(
        L"MoveFileW",
        lpExistingFileName,
        lpNewFileName,
        0,
        false
    );

    return MoveFileW(lpExistingFileName, lpNewFileName);
}

BOOL WINAPI myMoveFileExWHook(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    DWORD dwFlags)
{
    wcout << L"[Hook] MoveFileExW intercepted\n";
    wcout << L"  From: " << (lpExistingFileName ? lpExistingFileName : L"(null)") << endl;
    wcout << L"  To:   " << (lpNewFileName ? lpNewFileName : L"(null)") << endl;
    wcout << L"  Flags: " << dwFlags << endl;

    if (lpNewFileName && ContainsSuspiciousExtensionPattern(lpNewFileName))
    {
        wcout << L"[Defense] Suspicious ransomware extension change blocked." << endl;

        LogRenameAttempt(
            L"MoveFileExW",
            lpExistingFileName,
            lpNewFileName,
            dwFlags,
            true
        );

        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    LogRenameAttempt(
        L"MoveFileExW",
        lpExistingFileName,
        lpNewFileName,
        dwFlags,
        false
    );

    return MoveFileExW(lpExistingFileName, lpNewFileName, dwFlags);
}

BOOL WINAPI myMoveFileAHook(
    LPCSTR lpExistingFileName,
    LPCSTR lpNewFileName)
{
    string newPathA = lpNewFileName ? lpNewFileName : "";
    wstring newPathW(newPathA.begin(), newPathA.end());

    if (ContainsSuspiciousExtensionPattern(newPathW))
    {
        cout << "[Defense] Suspicious MoveFileA rename blocked." << endl;
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    return MoveFileA(lpExistingFileName, lpNewFileName);
}

BOOL WINAPI myMoveFileExAHook(
    LPCSTR lpExistingFileName,
    LPCSTR lpNewFileName,
    DWORD dwFlags)
{
    string newPathA = lpNewFileName ? lpNewFileName : "";
    wstring newPathW(newPathA.begin(), newPathA.end());

    if (ContainsSuspiciousExtensionPattern(newPathW))
    {
        cout << "[Defense] Suspicious MoveFileExA rename blocked." << endl;
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    return MoveFileExA(lpExistingFileName, lpNewFileName, dwFlags);
}

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(
    REMOTE_ENTRY_INFO * inRemoteInfo)
{
    cout << "[*] Anti-ransomware rename hook injected." << endl;

    HOOK_TRACE_INFO hMoveFileW = { NULL };
    HOOK_TRACE_INFO hMoveFileExW = { NULL };
    HOOK_TRACE_INFO hMoveFileA = { NULL };
    HOOK_TRACE_INFO hMoveFileExA = { NULL };

    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");

    FARPROC moveFileWAddr = GetProcAddress(hKernel32, "MoveFileW");
    FARPROC moveFileExWAddr = GetProcAddress(hKernel32, "MoveFileExW");
    FARPROC moveFileAAddr = GetProcAddress(hKernel32, "MoveFileA");
    FARPROC moveFileExAAddr = GetProcAddress(hKernel32, "MoveFileExA");

    if (FAILED(LhInstallHook(moveFileWAddr, myMoveFileWHook, nullptr, &hMoveFileW)))
        wcerr << L"[-] Failed to hook MoveFileW: " << RtlGetLastErrorString() << endl;
    else
        cout << "[+] MoveFileW hook installed." << endl;

    if (FAILED(LhInstallHook(moveFileExWAddr, myMoveFileExWHook, nullptr, &hMoveFileExW)))
        wcerr << L"[-] Failed to hook MoveFileExW: " << RtlGetLastErrorString() << endl;
    else
        cout << "[+] MoveFileExW hook installed." << endl;

    if (FAILED(LhInstallHook(moveFileAAddr, myMoveFileAHook, nullptr, &hMoveFileA)))
        wcerr << L"[-] Failed to hook MoveFileA: " << RtlGetLastErrorString() << endl;
    else
        cout << "[+] MoveFileA hook installed." << endl;

    if (FAILED(LhInstallHook(moveFileExAAddr, myMoveFileExAHook, nullptr, &hMoveFileExA)))
        wcerr << L"[-] Failed to hook MoveFileExA: " << RtlGetLastErrorString() << endl;
    else
        cout << "[+] MoveFileExA hook installed." << endl;

    ULONG ACLEntries[1] = { 0 };

    LhSetExclusiveACL(ACLEntries, 1, &hMoveFileW);
    LhSetExclusiveACL(ACLEntries, 1, &hMoveFileExW);
    LhSetExclusiveACL(ACLEntries, 1, &hMoveFileA);
    LhSetExclusiveACL(ACLEntries, 1, &hMoveFileExA);

    cout << "[*] Rename protection hooks are active." << endl;
}