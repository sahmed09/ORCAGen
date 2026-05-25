#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <easyhook.h>
#include <random>

#pragma comment(lib, "EasyHook32.lib")

using namespace std;

// Random generator for decoy filenames
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dist(0, 99);

// Decoy filename templates
const std::vector<std::string> kDecoyNames = {
    "api_key_backup.txt",
    "wallet_config.json",
    "credentials_temp.dat",
    "password_store.ini",
    "secret_config.conf",
    "browser_profile.txt",
    "config_api.json"
};

// Structure to track hooked FindFirstFile handles
struct FindFileContext {
    HANDLE realHandle;
    bool decoyReturned;
    int decoyIndex;
    std::string searchPath;
};

std::vector<FindFileContext*> g_FindContexts;

// Helper: Get directory from search path
std::string GetDirectoryFromSearchPath(const std::string& searchPath) {
    size_t pos = searchPath.find_last_of("\\/");
    if (pos != std::string::npos) {
        return searchPath.substr(0, pos);
    }
    return "";
}

// Helper: Check if this is a suspicious scan pattern
bool IsSuspiciousScan(const std::string& path) {
    std::string lower = path;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    // Check if scanning with wildcard patterns
    if (lower.find("\\*") != std::string::npos) {
        return true;
    }
    return false;
}

// Original function pointers
typedef HANDLE(WINAPI* FindFirstFileA_t)(LPCSTR, LPWIN32_FIND_DATAA);
typedef BOOL(WINAPI* FindNextFileA_t)(HANDLE, LPWIN32_FIND_DATAA);
typedef BOOL(WINAPI* FindClose_t)(HANDLE);

FindFirstFileA_t pOriginalFindFirstFileA = nullptr;
FindNextFileA_t pOriginalFindNextFileA = nullptr;
FindClose_t pOriginalFindClose = nullptr;

// Hook: FindFirstFileA
HANDLE WINAPI myFindFirstFileAHook(
    LPCSTR lpFileName,
    LPWIN32_FIND_DATAA lpFindFileData)
{
    std::string searchPath(lpFileName);

    // Call original function
    HANDLE hFind = FindFirstFileA(lpFileName, lpFindFileData);

    if (IsSuspiciousScan(searchPath)) {
        cout << "[Hook] FindFirstFileA intercepted: " << lpFileName << endl;

        if (hFind != INVALID_HANDLE_VALUE) {
            // Create context to track this search
            FindFileContext* ctx = new FindFileContext();
            ctx->realHandle = hFind;
            ctx->decoyReturned = false;
            ctx->decoyIndex = 0;
            ctx->searchPath = GetDirectoryFromSearchPath(searchPath);
            g_FindContexts.push_back(ctx);

            // Replace first result with decoy
            if (kDecoyNames.size() > 0) {
                strncpy_s(lpFindFileData->cFileName,
                    MAX_PATH,
                    kDecoyNames[0].c_str(),
                    _TRUNCATE);

                // Set file attributes to look real
                lpFindFileData->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
                lpFindFileData->nFileSizeLow = 1024 + (dist(gen) * 100);
                lpFindFileData->nFileSizeHigh = 0;

                ctx->decoyReturned = true;
                ctx->decoyIndex = 1;

                cout << "[Deception] Returning decoy file: " << lpFindFileData->cFileName << endl;
            }
        }
    }

    return hFind;
}

// Hook: FindNextFileA
BOOL WINAPI myFindNextFileAHook(
    HANDLE hFindFile,
    LPWIN32_FIND_DATAA lpFindFileData)
{
    // Find context for this handle
    FindFileContext* ctx = nullptr;
    for (auto& c : g_FindContexts) {
        if (c->realHandle == hFindFile) {
            ctx = c;
            break;
        }
    }

    if (ctx != nullptr && ctx->decoyIndex < kDecoyNames.size()) {
        // Return more decoys
        strncpy_s(lpFindFileData->cFileName,
            MAX_PATH,
            kDecoyNames[ctx->decoyIndex].c_str(),
            _TRUNCATE);

        lpFindFileData->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
        lpFindFileData->nFileSizeLow = 1024 + (dist(gen) * 100);
        lpFindFileData->nFileSizeHigh = 0;

        ctx->decoyIndex++;

        cout << "[Deception] FindNextFileA returning decoy: " << lpFindFileData->cFileName << endl;
        return TRUE;
    }

    // Call original for legitimate operations or after decoys exhausted
    BOOL result = FindNextFileA(hFindFile, lpFindFileData);

    if (!result && ctx != nullptr) {
        // Search complete, clean up context
        auto it = std::find(g_FindContexts.begin(), g_FindContexts.end(), ctx);
        if (it != g_FindContexts.end()) {
            delete ctx;
            g_FindContexts.erase(it);
        }
    }

    return result;
}

// Hook: FindClose
BOOL WINAPI myFindCloseHook(HANDLE hFindFile)
{
    // Clean up our context
    FindFileContext* ctx = nullptr;
    for (auto& c : g_FindContexts) {
        if (c->realHandle == hFindFile) {
            ctx = c;
            break;
        }
    }

    if (ctx != nullptr) {
        auto it = std::find(g_FindContexts.begin(), g_FindContexts.end(), ctx);
        if (it != g_FindContexts.end()) {
            delete ctx;
            g_FindContexts.erase(it);
        }
    }

    return FindClose(hFindFile);
}

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
    cout << "[*] Anti-CredentialScanner Injection started." << endl;

    HOOK_TRACE_INFO hFindFirstHook = { NULL };
    HOOK_TRACE_INFO hFindNextHook = { NULL };
    HOOK_TRACE_INFO hFindCloseHook = { NULL };

    // Get function addresses
    HMODULE hKernel32 = GetModuleHandle(TEXT("kernel32"));

    FARPROC findFirstAddr = GetProcAddress(hKernel32, "FindFirstFileA");
    FARPROC findNextAddr = GetProcAddress(hKernel32, "FindNextFileA");
    FARPROC findCloseAddr = GetProcAddress(hKernel32, "FindClose");

    cout << "[*] Installing hooks..." << endl;

    // Install FindFirstFileA hook
    NTSTATUS result = LhInstallHook(findFirstAddr, myFindFirstFileAHook, nullptr, &hFindFirstHook);
    if (FAILED(result)) {
        wcerr << L"[!] Failed to hook FindFirstFileA: " << RtlGetLastErrorString() << endl;
    }
    else {
        cout << "[+] FindFirstFileA hooked successfully!" << endl;
    }

    // Install FindNextFileA hook
    result = LhInstallHook(findNextAddr, myFindNextFileAHook, nullptr, &hFindNextHook);
    if (FAILED(result)) {
        wcerr << L"[!] Failed to hook FindNextFileA: " << RtlGetLastErrorString() << endl;
    }
    else {
        cout << "[+] FindNextFileA hooked successfully!" << endl;
    }

    // Install FindClose hook
    result = LhInstallHook(findCloseAddr, myFindCloseHook, nullptr, &hFindCloseHook);
    if (FAILED(result)) {
        wcerr << L"[!] Failed to hook FindClose: " << RtlGetLastErrorString() << endl;
    }
    else {
        cout << "[+] FindClose hooked successfully!" << endl;
    }

    // Enable all hooks for all threads
    ULONG ACLEntries[1] = { 0 };
    LhSetExclusiveACL(ACLEntries, 1, &hFindFirstHook);
    LhSetExclusiveACL(ACLEntries, 1, &hFindNextHook);
    LhSetExclusiveACL(ACLEntries, 1, &hFindCloseHook);

    cout << "[*] Hooks activated. Monitoring file system access..." << endl;
}