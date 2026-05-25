#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <easyhook.h>

#pragma comment(lib, "EasyHook32.lib")

using namespace std;

// Random generator for deception
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dist(0, 99);

// Store original function pointers
HANDLE(WINAPI* OriginalFindFirstFileW)(LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindData) = nullptr;
BOOL(WINAPI* OriginalFindNextFileW)(HANDLE hFindFile, LPWIN32_FIND_DATAW lpFindData) = nullptr;

// Hook: FindFirstFileW - Return fake failure for sensitive file searches
HANDLE WINAPI myFindFirstFileWHook(LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindData)
{
    wcout << L"[Hook] FindFirstFileW intercepted: " << lpFileName << endl;

    // Sleep(1000);

    // Check if this is a search in our target directory that might return sensitive files
    wstring searchPath = lpFileName;
    size_t lastSlash = searchPath.find_last_of(L"\\");
    if (lastSlash != wstring::npos) {
        wstring directory = searchPath.substr(0, lastSlash + 1);
        // If this is our target directory or a subdirectory, apply fake failure
        if (directory.find(L"Debug") != wstring::npos ||
            directory.find(L"Documents") != wstring::npos) {

            // Random chance to apply deception (80% chance)
            int chance = dist(gen);
            if (chance < 80) {
                wcout << L"[FakeFailure] Simulating search interruption for sensitive file access" << endl;
                SetLastError(ERROR_FILE_NOT_FOUND);
                return INVALID_HANDLE_VALUE;
            }
        }
    }

    // Call original function for non-sensitive searches
    return OriginalFindFirstFileW(lpFileName, lpFindData);
}

// Hook: FindNextFileW - Return fake failure for sensitive file searches
BOOL WINAPI myFindNextFileWHook(HANDLE hFindFile, LPWIN32_FIND_DATAW lpFindData)
{
    wcout << L"[Hook] FindNextFileW intercepted" << endl;

    // Sleep(1000);

    // Always return false to simulate search interruption
    int chance = dist(gen);
    if (chance < 80) {
        wcout << L"[FakeFailure] Simulating search interruption for sensitive file access" << endl;
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    // Call original function for non-sensitive searches
    return OriginalFindNextFileW(hFindFile, lpFindData);
}

// Hook: CreateFileW - Intercept file access operations with fake failure
HANDLE WINAPI myCreateFileWHook(
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile)
{
    wcout << L"[Hook] CreateFileW intercepted: " << lpFileName << endl;

    // Sleep(1000);

    // Check if this is a sensitive file access
    wstring fileName = lpFileName;
    std::transform(fileName.begin(), fileName.end(), fileName.begin(), ::tolower);

    // Common keywords for sensitive files
    const vector<wstring> sensitiveKeywords = {
        L"api_key", L"secret", L"password", L"credential",
        L"wallet", L"key", L"config", L"settings", L"login",
        L"pass", L"auth", L"token", L"db", L"database"
    };

    // Check if file name contains sensitive keywords
    bool isSensitive = false;
    for (const auto& keyword : sensitiveKeywords) {
        if (fileName.find(keyword) != wstring::npos) {
            isSensitive = true;
            break;
        }
    }

    // Apply fake failure to sensitive files
    if (isSensitive) {
        int chance = dist(gen);
        if (chance < 80) { // 80% chance to simulate failure
            wcout << L"[FakeFailure] Simulating access interruption for sensitive file: " << lpFileName << endl;
            SetLastError(ERROR_ACCESS_DENIED);
            return INVALID_HANDLE_VALUE;
        }
    }

    // Call original function for non-sensitive files
    return CreateFileW(lpFileName,
        dwDesiredAccess,
        dwShareMode,
        lpSecurityAttributes,
        dwCreationDisposition,
        dwFlagsAndAttributes,
        hTemplateFile);
}

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
    cout << "[*] Injection started." << endl;

    HOOK_TRACE_INFO hFindFirstHook = { NULL };
    HOOK_TRACE_INFO hFindNextHook = { NULL };
    HOOK_TRACE_INFO hCreateFileHook = { NULL };

    // Get original function addresses
    FARPROC findFirstAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "FindFirstFileW");
    FARPROC findNextAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "FindNextFileW");
    FARPROC createFileAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "CreateFileW");

    // Store original functions
    OriginalFindFirstFileW = (HANDLE(WINAPI*)(LPCWSTR, LPWIN32_FIND_DATAW))findFirstAddr;
    OriginalFindNextFileW = (BOOL(WINAPI*)(HANDLE, LPWIN32_FIND_DATAW))findNextAddr;

    // Install hooks
    LhInstallHook(findFirstAddr, myFindFirstFileWHook, nullptr, &hFindFirstHook);
    LhInstallHook(findNextAddr, myFindNextFileWHook, nullptr, &hFindNextHook);
    LhInstallHook(createFileAddr, myCreateFileWHook, nullptr, &hCreateFileHook);

    // Enable all hooks
    ULONG ACLEntries[1] = { 0 };
    LhSetExclusiveACL(ACLEntries, 1, &hFindFirstHook);
    LhSetExclusiveACL(ACLEntries, 1, &hFindNextHook);
    LhSetExclusiveACL(ACLEntries, 1, &hCreateFileHook);

    cout << "[*] All hooks installed successfully." << endl;
}