#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <easyhook.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>

using namespace std;

// Suspicious extensions (fixed and dynamic patterns)
vector<wstring> suspiciousExtensions = {
    L".locked",
    L".enc",
    L".revil",
    L".cry",
    L".wannacry",
    L".crypt",
    L".encrypted*" // Match extensions like ".encrypted.<base64>"
};

// Check if filename ends with suspicious pattern
bool IsSuspiciousExtension(const wstring& fileName)
{
    wstring lowerFileName = fileName;
    transform(lowerFileName.begin(), lowerFileName.end(), lowerFileName.begin(), ::towlower);

    for (const auto& ext : suspiciousExtensions)
    {
        wstring lowerExt = ext;
        transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(), ::towlower);

        if (lowerExt.back() == L'*') {
            wstring prefix = lowerExt.substr(0, lowerExt.length() - 1);
            if (lowerFileName.find(prefix) != wstring::npos)
                return true;
        }
        else if (lowerFileName.length() >= lowerExt.length()) {
            if (lowerFileName.compare(lowerFileName.length() - lowerExt.length(), lowerExt.length(), lowerExt) == 0)
                return true;
        }
    }
    return false;
}

// Helper to extract just filename from full path
wstring ExtractFileName(const wstring& fullPath)
{
    size_t lastSlash = fullPath.find_last_of(L"\\/");
    if (lastSlash != wstring::npos)
        return fullPath.substr(lastSlash + 1);
    return fullPath;
}

// Log rename attempts
void LogAttempt(const wstring& oldName, const wstring& newName)
{
    wofstream logFile(L"rename_attempts.log", ios::app);
    logFile << L"[BLOCKED] Rename attempt: " << oldName << L" -> " << newName << endl;
    logFile.close();
}

// --- Hooked Functions ---

BOOL WINAPI MyMoveFileExWHook(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, DWORD dwFlags)
{
    wstring newName = ExtractFileName(lpNewFileName);

    if (IsSuspiciousExtension(newName)) {
        wcout << L"[BLOCKED] MoveFileExW: " << lpExistingFileName << L" -> " << lpNewFileName << endl;
        LogAttempt(lpExistingFileName, lpNewFileName);
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }
    return MoveFileExW(lpExistingFileName, lpNewFileName, dwFlags);
}

BOOL WINAPI MyMoveFileExAHook(LPCSTR lpExistingFileName, LPCSTR lpNewFileName, DWORD dwFlags)
{
    wstring wideNew = L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, lpNewFileName, -1, NULL, 0);
    if (len > 0) {
        wideNew.resize(len - 1);
        MultiByteToWideChar(CP_UTF8, 0, lpNewFileName, -1, &wideNew[0], len);
    }

    if (IsSuspiciousExtension(ExtractFileName(wideNew))) {
        wcout << L"[BLOCKED] MoveFileExA: " << lpExistingFileName << L" -> " << lpNewFileName << endl;
        LogAttempt(wstring(lpExistingFileName, lpExistingFileName + strlen(lpExistingFileName)), wideNew);
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }
    return MoveFileExA(lpExistingFileName, lpNewFileName, dwFlags);
}

BOOL WINAPI MyMoveFileWHook(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName)
{
    wstring newName = ExtractFileName(lpNewFileName);

    if (IsSuspiciousExtension(newName)) {
        wcout << L"[BLOCKED] MoveFileW: " << lpExistingFileName << L" -> " << lpNewFileName << endl;
        LogAttempt(lpExistingFileName, lpNewFileName);
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }
    return MoveFileW(lpExistingFileName, lpNewFileName);
}

BOOL WINAPI MyMoveFileAHook(LPCSTR lpExistingFileName, LPCSTR lpNewFileName)
{
    wstring wideNew = L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, lpNewFileName, -1, NULL, 0);
    if (len > 0) {
        wideNew.resize(len - 1);
        MultiByteToWideChar(CP_UTF8, 0, lpNewFileName, -1, &wideNew[0], len);
    }

    if (IsSuspiciousExtension(ExtractFileName(wideNew))) {
        wcout << L"[BLOCKED] MoveFileA: " << lpExistingFileName << L" -> " << lpNewFileName << endl;
        LogAttempt(wstring(lpExistingFileName, lpExistingFileName + strlen(lpExistingFileName)), wideNew);
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }
    return MoveFileA(lpExistingFileName, lpNewFileName);
}

// --- DLL Entry Point ---

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
    cout << "[*] Injection started." << endl;

    HOOK_TRACE_INFO hMoveFileExWHook = { NULL };
    HOOK_TRACE_INFO hMoveFileExAHook = { NULL };
    HOOK_TRACE_INFO hMoveFileWHook = { NULL };
    HOOK_TRACE_INFO hMoveFileAHook = { NULL };

    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");

    LhInstallHook(GetProcAddress(hKernel32, "MoveFileExW"), MyMoveFileExWHook, nullptr, &hMoveFileExWHook);
    LhInstallHook(GetProcAddress(hKernel32, "MoveFileExA"), MyMoveFileExAHook, nullptr, &hMoveFileExAHook);
    LhInstallHook(GetProcAddress(hKernel32, "MoveFileW"), MyMoveFileWHook, nullptr, &hMoveFileWHook);
    LhInstallHook(GetProcAddress(hKernel32, "MoveFileA"), MyMoveFileAHook, nullptr, &hMoveFileAHook);

    ULONG ACLEntries[1] = { 0 };
    LhSetExclusiveACL(ACLEntries, 1, &hMoveFileExWHook);
    LhSetExclusiveACL(ACLEntries, 1, &hMoveFileExAHook);
    LhSetExclusiveACL(ACLEntries, 1, &hMoveFileWHook);
    LhSetExclusiveACL(ACLEntries, 1, &hMoveFileAHook);

    cout << "[*] All rename-related hooks installed." << endl;
}
