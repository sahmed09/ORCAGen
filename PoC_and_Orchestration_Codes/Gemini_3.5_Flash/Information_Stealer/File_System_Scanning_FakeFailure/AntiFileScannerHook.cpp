#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <easyhook.h>
#include <random>

#pragma comment(lib, "EasyHook32.lib") // Swap to EasyHook64.lib if building x64

using namespace std;

// Thread-safe random engine setup for generating dynamic honeyfile names
static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_int_distribution<> dist(1000, 9999);

// Function pointer signatures for invoking the original, unhooked Windows APIs
typedef HANDLE(WINAPI* PFN_FindFirstFileW)(LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData);
typedef BOOL(WINAPI* PFN_FindNextFileW)(HANDLE hFindFile, LPWIN32_FIND_DATAW lpFindFileData);

// Helper helper function to determine if a found file contains a sensitive string
bool IsSensitiveString(const std::wstring& filename)
{
    std::vector<std::wstring> targets = {
        L".json", L".conf", L".ini", L".txt",
        L"api_key", L"wallet.dat", L"profile"
    };

    std::wstring lowerFilename = filename;
    for (auto& c : lowerFilename) c = towlower(c);

    for (const auto& target : targets)
    {
        if (lowerFilename.find(target) != std::wstring::npos)
        {
            return true;
        }
    }
    return false;
}

// Manipulates the file structural metadata to pass back a harmless decoy name
void ApplyDeception(LPWIN32_FIND_DATAW lpFindFileData)
{
    if (lpFindFileData == nullptr) return;

    std::wstring currentName = lpFindFileData->cFileName;

    // Do not alter relative directory traversal pointers
    if (currentName == L"." || currentName == L"..") return;

    if (IsSensitiveString(currentName))
    {
        wstring decoyName = L"decoy_backup_" + std::to_wstring(dist(gen)) + L".bak";

        wcout << L"[DECEPTION] Intercepted sensitive file: " << currentName
            << L" -> Masked as: " << decoyName << endl;

        // Clear out old buffer securely and replace with safe token string
        SecureZeroMemory(lpFindFileData->cFileName, sizeof(lpFindFileData->cFileName));
        wcscpy_s(lpFindFileData->cFileName, MAX_PATH, decoyName.c_str());

        // Optional: Poison the file sizes so the malware gets empty structural definitions
        lpFindFileData->nFileSizeHigh = 0;
        lpFindFileData->nFileSizeLow = 0;
    }
}

// Hook handler for FindFirstFileW
HANDLE WINAPI myFindFirstFileWHook(LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData)
{
    // Pass execution down to the underlying real OS API first
    HANDLE hResult = FindFirstFileW(lpFileName, lpFindFileData);

    if (hResult != INVALID_HANDLE_VALUE)
    {
        ApplyDeception(lpFindFileData);
    }

    return hResult;
}

// Hook handler for FindNextFileW
BOOL WINAPI myFindNextFileWHook(HANDLE hFindFile, LPWIN32_FIND_DATAW lpFindFileData)
{
    // Pass execution down to the underlying real OS API first
    BOOL bResult = FindNextFileW(hFindFile, lpFindFileData);

    if (bResult)
    {
        ApplyDeception(lpFindFileData);
    }

    return bResult;
}

// Primary EasyHook initialization framework
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
    cout << "\n[*] Cyber Deception Engine Active: Intercepting File Operations.\n";

    HOOK_TRACE_INFO hFindFirstFile = { NULL };
    HOOK_TRACE_INFO hFindNextFile = { NULL };

    // Resolve structural addresses for the file system APIs inside kernelbase or kernel32
    FARPROC findFirstAddr = GetProcAddress(GetModuleHandle(TEXT("kernelbase")), "FindFirstFileW");
    if (!findFirstAddr) findFirstAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "FindFirstFileW");

    FARPROC findNextAddr = GetProcAddress(GetModuleHandle(TEXT("kernelbase")), "FindNextFileW");
    if (!findNextAddr) findNextAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "FindNextFileW");

    // Install inline runtime detours
    NTSTATUS statusFirst = LhInstallHook(findFirstAddr, myFindFirstFileWHook, nullptr, &hFindFirstFile);
    NTSTATUS statusNext = LhInstallHook(findNextAddr, myFindNextFileWHook, nullptr, &hFindNextFile);

    if (FAILED(statusFirst) || FAILED(statusNext))
    {
        wcerr << L"[-] Critical Error: Failed to initialize file system deception hooks." << endl;
        return;
    }

    // Configure the Access Control List locally for this process context
    ULONG ACLEntries[1] = { 0 };
    LhSetExclusiveACL(ACLEntries, 1, &hFindFirstFile);
    LhSetExclusiveACL(ACLEntries, 1, &hFindNextFile);

    cout << "[+] File system abstraction hooks installed smoothly.\n" << endl;
}