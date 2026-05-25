#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <easyhook.h>
#include <thread>
#include <chrono>

#pragma comment(lib, "EasyHook32.lib")

using namespace std;

// Random generator for deception
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dist(0, 99);

// Decoy file names to return instead of real sensitive files
const vector<wstring> decoyFiles = {
    L"decoy_config.ini",
    L"fake_credentials.txt",
    L"dummy_api_key.json",
    L"test_wallet.dat",
    L"sample_secret.conf",
    L"temp_settings.xml",
    L"backup_login.txt",
    L"private_key.pem",
    L"auth_token.json",
    L"credential_store.ini"
};

// Store original FindFirstFileW function pointer
HANDLE(WINAPI* OriginalFindFirstFileW)(LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindData) = nullptr;

// Hook: FindFirstFileW - Intercept file searches and return decoy files
HANDLE WINAPI myFindFirstFileWHook(LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindData)
{
    wcout << L"[Hook] FindFirstFileW intercepted: " << lpFileName << endl;

    // Introduce artificial delay to increase scan time
    // Sleep(500);

    // Call the original function to get real results
    HANDLE hResult = OriginalFindFirstFileW(lpFileName, lpFindData);

    // Check if this is a search in our target directory that might return sensitive files
    wstring searchPath = lpFileName;
    size_t lastSlash = searchPath.find_last_of(L"\\");
    if (lastSlash != wstring::npos) {
        wstring directory = searchPath.substr(0, lastSlash + 1);
        // If this is our target directory or a subdirectory, apply deception
        if (directory.find(L"Debug") != wstring::npos ||
            directory.find(L"Documents") != wstring::npos) {

            // Random chance to apply deception (80% chance)
            int chance = dist(gen);
            // if (chance < 80 && hResult != INVALID_HANDLE_VALUE) {
            if (hResult != INVALID_HANDLE_VALUE) {
                // Return a decoy file instead of real sensitive files
                // This will make the malware think it found something but actually gets fake data
                wcout << L"[Deception] Replacing real search results with decoy files" << endl;

                // Get a random decoy file name
                size_t decoyIndex = dist(gen) % decoyFiles.size();
                wstring decoyFileName = decoyFiles[decoyIndex];

                // Fill the find data with decoy information
                wcscpy_s(lpFindData->cFileName, MAX_PATH, decoyFileName.c_str());
                wcscpy_s(lpFindData->cAlternateFileName, 14, L"DECOY.DAT");

                // Set file attributes to make it appear like a real file
                lpFindData->dwFileAttributes = FILE_ATTRIBUTE_ARCHIVE;
                lpFindData->ftCreationTime.dwLowDateTime = 0x12345678;
                lpFindData->ftCreationTime.dwHighDateTime = 0x9ABCDEF0;
                lpFindData->ftLastAccessTime = lpFindData->ftCreationTime;
                lpFindData->ftLastWriteTime = lpFindData->ftCreationTime;
                lpFindData->nFileSizeHigh = 0;
                lpFindData->nFileSizeLow = 1024; // 1KB file size

                wcout << L"[Deception] Returning decoy file: " << decoyFileName << endl;
            }
        }
    }

    return hResult;
}

// Hook: FindNextFileW - Intercept subsequent file searches and return decoy files
BOOL WINAPI myFindNextFileWHook(HANDLE hFindFile, LPWIN32_FIND_DATAW lpFindData)
{
    wcout << L"[Hook] FindNextFileW intercepted" << endl;

    // Introduce artificial delay to increase scan time
    // Sleep(500);

    // Call the original function
    BOOL result = FindNextFileW(hFindFile, lpFindData);

    if (result) {
        // Apply deception with 50% chance
        int chance = dist(gen);
        // if (chance < 50) {
        if (result) {
            wcout << L"[Deception] Modifying search results to decoy files" << endl;

            // Get a random decoy file name
            size_t decoyIndex = dist(gen) % decoyFiles.size();
            wstring decoyFileName = decoyFiles[decoyIndex];

            // Fill the find data with decoy information
            wcscpy_s(lpFindData->cFileName, MAX_PATH, decoyFileName.c_str());
            wcscpy_s(lpFindData->cAlternateFileName, 14, L"DECOY.DAT");

            wcout << L"[Deception] Returning decoy file: " << decoyFileName << endl;
        }
    }

    return result;
}

// Hook: CreateFileW - Intercept file access operations
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

    // Introduce artificial delay to increase scan time
    // Sleep(500);

    // For demonstration, let's also apply deception here
    // int chance = dist(gen);
    // if (chance < 30) {
    wcout << L"[Deception] Modified CreateFileW access to: " << lpFileName << endl;
    // We still return the actual file handle but log that deception occurred
// }

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
    wcout << L"[*] Injection started." << endl;

    HOOK_TRACE_INFO hFindFirstFileHook = { NULL };
    HOOK_TRACE_INFO hFindNextFileHook = { NULL };
    HOOK_TRACE_INFO hCreateFileHook = { NULL };

    // Get original function addresses
    FARPROC findFirstAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "FindFirstFileW");
    FARPROC findNextAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "FindNextFileW");
    FARPROC createFileAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "CreateFileW");

    // Store original function pointers
    OriginalFindFirstFileW = (HANDLE(WINAPI*)(LPCWSTR, LPWIN32_FIND_DATAW))findFirstAddr;

    // Install hooks
    LhInstallHook(findFirstAddr, myFindFirstFileWHook, nullptr, &hFindFirstFileHook);
    LhInstallHook(findNextAddr, myFindNextFileWHook, nullptr, &hFindNextFileHook);
    LhInstallHook(createFileAddr, myCreateFileWHook, nullptr, &hCreateFileHook);

    // Enable all hooks
    ULONG ACLEntries[1] = { 0 };
    LhSetExclusiveACL(ACLEntries, 1, &hFindFirstFileHook);
    LhSetExclusiveACL(ACLEntries, 1, &hFindNextFileHook);
    LhSetExclusiveACL(ACLEntries, 1, &hCreateFileHook);

    wcout << L"[*] Hooks installed successfully." << endl;
}