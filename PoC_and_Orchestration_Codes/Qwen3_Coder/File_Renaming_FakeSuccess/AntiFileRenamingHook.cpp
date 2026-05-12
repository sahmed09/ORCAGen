#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <easyhook.h>
#include <vector>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <Psapi.h>

#pragma comment(lib, "EasyHook32.lib")
#pragma comment(lib, "Psapi.lib")

using namespace std;

// Suspicious extensions to monitor
const vector<wstring> SUSPICIOUS_EXTENSIONS = {
    L".locked", L".enc", L".revil", L".crypt", L".encrypted",
    L".ransom", L".locky", L".wannacry", L".cerber"
};

// Whitelisted processes (legitimate applications)
const vector<wstring> WHITELISTED_PROCESSES = {
    L"explorer.exe", L"notepad.exe", L"word.exe", L"excel.exe",
    L"powerpnt.exe", L"chrome.exe", L"firefox.exe", L"vscode.exe"
};

// Check if extension is suspicious
bool IsSuspiciousExtension(const wstring& fileName) {
    size_t dotPos = fileName.find_last_of(L'.');
    if (dotPos == wstring::npos) return false;

    wstring ext = fileName.substr(dotPos);
    transform(ext.begin(), ext.end(), ext.begin(), ::towlower);

    for (const auto& suspicious : SUSPICIOUS_EXTENSIONS) {
        if (ext == suspicious) return true;
    }
    return false;
}

// Check if file path is in system directory
bool IsSystemDirectory(const wstring& filePath) {
    wchar_t sysDir[MAX_PATH];
    GetSystemDirectoryW(sysDir, MAX_PATH);
    wstring path = sysDir;
    path += L"\\";

    return filePath.find(path) == 0;
}

// Log ransomware behavior
void LogSuspiciousActivity(const wstring& operation, const wstring& fileName, const wstring& processName) {
    try {
        wofstream logFile(L"ransomware_detection.log", ios::app);
        if (logFile.is_open()) {
            SYSTEMTIME st;
            GetLocalTime(&st);
            logFile << L"[" << st.wYear << L"-" << st.wMonth << L"-" << st.wDay << L" "
                << st.wHour << L":" << st.wMinute << L":" << st.wSecond << L"] "
                << operation << L": " << fileName << L" (Process: " << processName << L")" << endl;
            logFile.close();
        }
    }
    catch (...) {
        // Silently fail
    }
}

// Common logic for all rename hooks
BOOL HandleSuspiciousRename(const wstring& apiName, const wstring& existingFile, const wstring& newFile) {
    if (IsSuspiciousExtension(newFile)) {
        wchar_t processName[MAX_PATH] = L"Unknown";
        DWORD pid = GetCurrentProcessId();
        HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
        if (hProc) {
            GetModuleBaseNameW(hProc, NULL, processName, MAX_PATH);
            CloseHandle(hProc);
        }

        LogSuspiciousActivity(apiName, newFile, processName);

        wcout << L"[BLOCKED] Suspicious rename detected: " << existingFile
            << L" -> " << newFile << endl;
        wcout << L"[Deception] Returning fake success for " << apiName << endl;

        return TRUE;  // FAKE SUCCESS to deceive ransomware
    }
    return FALSE; // Not suspicious
}

// Hook: MoveFileW
BOOL WINAPI myMoveFileWHook(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName) {
    wcout << L"[Hook] MoveFileW intercepted" << endl;

    if (lpNewFileName && HandleSuspiciousRename(L"MoveFileW", lpExistingFileName, lpNewFileName)) {
        return TRUE;  // Fake success
    }

    return MoveFileW(lpExistingFileName, lpNewFileName);
}

// Hook: MoveFileExW
BOOL WINAPI myMoveFileExWHook(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, DWORD dwFlags) {
    wcout << L"[Hook] MoveFileExW intercepted" << endl;

    if (lpNewFileName && HandleSuspiciousRename(L"MoveFileExW", lpExistingFileName, lpNewFileName)) {
        return TRUE;  // Fake success
    }

    return MoveFileExW(lpExistingFileName, lpNewFileName, dwFlags);
}

// Hook: RenameFile (fallback — uses MoveFileW internally)
BOOL WINAPI myRenameFileHook(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName) {
    wcout << L"[Hook] RenameFile intercepted" << endl;

    if (lpNewFileName && HandleSuspiciousRename(L"RenameFile", lpExistingFileName, lpNewFileName)) {
        return TRUE;  // Fake success
    }

    return MoveFileW(lpExistingFileName, lpNewFileName);
}

// Hook installer
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo) {
    cout << "[*] Ransomware FakeRename Defense started." << endl;

    HOOK_TRACE_INFO hMoveFileHook = { NULL };
    HOOK_TRACE_INFO hMoveFileExHook = { NULL };
    HOOK_TRACE_INFO hRenameFileHook = { NULL };

    FARPROC moveFileAddr = GetProcAddress(GetModuleHandleW(L"kernel32"), "MoveFileW");
    FARPROC moveFileExAddr = GetProcAddress(GetModuleHandleW(L"kernel32"), "MoveFileExW");
    FARPROC renameFileAddr = GetProcAddress(GetModuleHandleW(L"kernel32"), "RenameFile");  // optional, fallback

    if (moveFileAddr)
        LhInstallHook(moveFileAddr, myMoveFileWHook, nullptr, &hMoveFileHook);
    if (moveFileExAddr)
        LhInstallHook(moveFileExAddr, myMoveFileExWHook, nullptr, &hMoveFileExHook);
    if (renameFileAddr)
        LhInstallHook(renameFileAddr, myRenameFileHook, nullptr, &hRenameFileHook);

    ULONG ACLEntries[1] = { 0 };
    if (moveFileAddr)
        LhSetExclusiveACL(ACLEntries, 1, &hMoveFileHook);
    if (moveFileExAddr)
        LhSetExclusiveACL(ACLEntries, 1, &hMoveFileExHook);
    if (renameFileAddr)
        LhSetExclusiveACL(ACLEntries, 1, &hRenameFileHook);

    cout << "[+] All ransomware rename hooks installed and active." << endl;
}
