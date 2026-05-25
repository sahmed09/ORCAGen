#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <easyhook.h>

#pragma comment(lib, "EasyHook32.lib") // Change to EasyHook64.lib if compiling x64

using namespace std;

// Blacklist of suspicious ransomware extensions to intercept and block
const vector<wstring> RANSOMWARE_EXTENSIONS = {
    L".locked",
    L".enc",
    L".revil",
    L".encrypted",
    L".crypto",
    L".ryuk"
};

// Typedef for the original MoveFileWithProgressW function signature
typedef BOOL(WINAPI* MoveFileWithProgressW_t)(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    LPPROGRESS_ROUTINE lpProgressRoutine,
    LPVOID lpData,
    DWORD dwFlags
    );

// Helper function to extract and lowercase the file extension
wstring GetFileExtension(const wstring& filePath) {
    size_t dotIdx = filePath.find_last_of(L".");
    if (dotIdx == wstring::npos || dotIdx == filePath.length() - 1) {
        return L"";
    }

    // Check if the path separator follows the dot, meaning it's a directory dot
    size_t sepIdx = filePath.find_last_of(L"\\/");
    if (sepIdx != wstring::npos && sepIdx > dotIdx) {
        return L"";
    }

    wstring ext = filePath.substr(dotIdx);
    // Convert to lowercase to prevent case-folding bypasses (e.g., .LOCKED)
    transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
    return ext;
}

// Detour Function: The replacement hook for MoveFileWithProgressW
BOOL WINAPI myMoveFileWithProgressWHook(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    LPPROGRESS_ROUTINE lpProgressRoutine,
    LPVOID lpData,
    DWORD dwFlags)
{
    if (lpNewFileName != nullptr) {
        wstring destPath(lpNewFileName);
        wstring extension = GetFileExtension(destPath);

        // Scan the target extension against our behavioral blacklist
        for (const auto& targetBadExt : RANSOMWARE_EXTENSIONS) {
            if (extension == targetBadExt) {
                wcout << L"\n[Active Defense] CRITICAL: Intercepted suspicious rename operation!" << endl;
                wcout << L"    Source: " << lpExistingFileName << endl;
                wcout << L"    Target: " << lpNewFileName << endl;
                wcout << L"    Action: BLOCKING OPERATION (Ransomware Signatures Matched)" << endl;

                // Emulate standard Windows authorization failure behavior
                SetLastError(ERROR_ACCESS_DENIED);
                return FALSE;
            }
        }
    }

    // Pass-through execution for normal, benign application requests
    return MoveFileWithProgressW(
        lpExistingFileName,
        lpNewFileName,
        lpProgressRoutine,
        lpData,
        dwFlags
    );
}

// System Injection Point called by EasyHook
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
    cout << "\n[*] Cyber Deception Framework: Active Defense Module Engaged." << endl;

    HOOK_TRACE_INFO hMoveFileHook = { NULL };

    // Resolve the address of MoveFileWithProgressW from kernel32
    FARPROC moveFileAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "MoveFileWithProgressW");

    if (moveFileAddr == nullptr) {
        cerr << "[-] Error: Failed to locate MoveFileWithProgressW address." << endl;
        return;
    }

    // Install the trampoline detour
    NTSTATUS result = LhInstallHook(
        moveFileAddr,
        myMoveFileWithProgressWHook,
        nullptr,
        &hMoveFileHook
    );

    if (FAILED(result)) {
        wstring errStr(RtlGetLastErrorString());
        wcerr << L"[-] Failed to deploy File System hook: " << errStr << endl;
        return;
    }
    else {
        cout << "[+] File System active monitoring deployment successful!" << endl;
    }

    // Apply the tracking rules via the Access Control List (ACL)
    ULONG ACLEntries[1] = { 0 };
    LhSetExclusiveACL(ACLEntries, 1, &hMoveFileHook);
}