#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <easyhook.h>

#pragma comment(lib, "EasyHook32.lib") // Use EasyHook64.lib for x64 builds

using namespace std;

static const vector<wstring> SUSPICIOUS_EXTENSIONS = {
    L".locked",
    L".enc",
    L".encrypted",
    L".revil",
    L".REvil"
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

bool IsSuspiciousRenameTarget(LPCWSTR newFileName)
{
    if (newFileName == nullptr)
        return false;

    wstring path(newFileName);

    for (const auto& ext : SUSPICIOUS_EXTENSIONS)
    {
        if (EndsWithIgnoreCase(path, ext))
            return true;
    }

    return false;
}

void LogRenameAttempt(
    const wstring& apiName,
    LPCWSTR oldName,
    LPCWSTR newName,
    bool blocked)
{
    wofstream logFile(L"rename_hook_log.txt", ios::app);

    if (!logFile.is_open())
        return;

    logFile << L"[" << apiName << L"]\n";
    logFile << L"Old Path: " << (oldName ? oldName : L"(null)") << L"\n";
    logFile << L"New Path: " << (newName ? newName : L"(null)") << L"\n";
    logFile << L"Action: " << (blocked ? L"BLOCKED + FAKE SUCCESS" : L"ALLOWED") << L"\n";
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

    if (IsSuspiciousRenameTarget(lpNewFileName))
    {
        wcout << L"[Defense] Suspicious ransomware-style rename blocked." << endl;
        LogRenameAttempt(L"MoveFileW", lpExistingFileName, lpNewFileName, true);

        SetLastError(ERROR_SUCCESS);
        return TRUE; // Fake success to deceive ransomware
    }

    LogRenameAttempt(L"MoveFileW", lpExistingFileName, lpNewFileName, false);

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

    if (IsSuspiciousRenameTarget(lpNewFileName))
    {
        wcout << L"[Defense] Suspicious ransomware-style extension change blocked." << endl;
        LogRenameAttempt(L"MoveFileExW", lpExistingFileName, lpNewFileName, true);

        SetLastError(ERROR_SUCCESS);
        return TRUE; // Fake success
    }

    LogRenameAttempt(L"MoveFileExW", lpExistingFileName, lpNewFileName, false);

    return MoveFileExW(lpExistingFileName, lpNewFileName, dwFlags);
}

BOOL WINAPI myMoveFileAHook(
    LPCSTR lpExistingFileName,
    LPCSTR lpNewFileName)
{
    string newPath = lpNewFileName ? lpNewFileName : "";
    wstring wideNewPath(newPath.begin(), newPath.end());

    if (IsSuspiciousRenameTarget(wideNewPath.c_str()))
    {
        cout << "[Defense] Suspicious MoveFileA rename blocked + fake success." << endl;
        SetLastError(ERROR_SUCCESS);
        return TRUE;
    }

    return MoveFileA(lpExistingFileName, lpNewFileName);
}

BOOL WINAPI myMoveFileExAHook(
    LPCSTR lpExistingFileName,
    LPCSTR lpNewFileName,
    DWORD dwFlags)
{
    string newPath = lpNewFileName ? lpNewFileName : "";
    wstring wideNewPath(newPath.begin(), newPath.end());

    if (IsSuspiciousRenameTarget(wideNewPath.c_str()))
    {
        cout << "[Defense] Suspicious MoveFileExA rename blocked + fake success." << endl;
        SetLastError(ERROR_SUCCESS);
        return TRUE;
    }

    return MoveFileExA(lpExistingFileName, lpNewFileName, dwFlags);
}

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(
    REMOTE_ENTRY_INFO * inRemoteInfo)
{
    cout << "[*] Anti-ransomware rename hook DLL injected." << endl;

    HOOK_TRACE_INFO hMoveFileWHook = { NULL };
    HOOK_TRACE_INFO hMoveFileExWHook = { NULL };
    HOOK_TRACE_INFO hMoveFileAHook = { NULL };
    HOOK_TRACE_INFO hMoveFileExAHook = { NULL };

    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");

    FARPROC moveFileWAddr = GetProcAddress(hKernel32, "MoveFileW");
    FARPROC moveFileExWAddr = GetProcAddress(hKernel32, "MoveFileExW");
    FARPROC moveFileAAddr = GetProcAddress(hKernel32, "MoveFileA");
    FARPROC moveFileExAAddr = GetProcAddress(hKernel32, "MoveFileExA");

    if (FAILED(LhInstallHook(moveFileWAddr, myMoveFileWHook, nullptr, &hMoveFileWHook)))
    {
        wcerr << L"[-] Failed to hook MoveFileW: " << RtlGetLastErrorString() << endl;
    }
    else
    {
        cout << "[+] MoveFileW hook installed." << endl;
    }

    if (FAILED(LhInstallHook(moveFileExWAddr, myMoveFileExWHook, nullptr, &hMoveFileExWHook)))
    {
        wcerr << L"[-] Failed to hook MoveFileExW: " << RtlGetLastErrorString() << endl;
    }
    else
    {
        cout << "[+] MoveFileExW hook installed." << endl;
    }

    if (FAILED(LhInstallHook(moveFileAAddr, myMoveFileAHook, nullptr, &hMoveFileAHook)))
    {
        wcerr << L"[-] Failed to hook MoveFileA: " << RtlGetLastErrorString() << endl;
    }
    else
    {
        cout << "[+] MoveFileA hook installed." << endl;
    }

    if (FAILED(LhInstallHook(moveFileExAAddr, myMoveFileExAHook, nullptr, &hMoveFileExAHook)))
    {
        wcerr << L"[-] Failed to hook MoveFileExA: " << RtlGetLastErrorString() << endl;
    }
    else
    {
        cout << "[+] MoveFileExA hook installed." << endl;
    }

    ULONG ACLEntries[1] = { 0 };

    LhSetExclusiveACL(ACLEntries, 1, &hMoveFileWHook);
    LhSetExclusiveACL(ACLEntries, 1, &hMoveFileExWHook);
    LhSetExclusiveACL(ACLEntries, 1, &hMoveFileAHook);
    LhSetExclusiveACL(ACLEntries, 1, &hMoveFileExAHook);

    cout << "[*] Rename defense hooks are active." << endl;
}