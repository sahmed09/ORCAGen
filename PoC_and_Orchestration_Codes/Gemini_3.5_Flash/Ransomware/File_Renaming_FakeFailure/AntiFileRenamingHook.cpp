#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <easyhook.h>

#pragma comment(lib, "EasyHook32.lib") // Change to EasyHook64.lib if building for 64-bit platforms

using namespace std;

// --- Cyber Deception Signature Database ---
// List of targeted ransomware extensions to watch out for
const vector<wstring> SUSPICIOUS_EXTENSIONS = {
    L".locked", L".enc", L".revil", L".encrypted",
    L".wannacry", L".crypto", L".locky", L".crypted"
};

// --- Helper Functions ---
// Evaluates path inputs to catch specific target extensions (Case-Insensitive)
bool IsSuspiciousExtension(const wstring& filePath) {
    if (filePath.empty()) return false;

    wstring lowerPath = filePath;
    transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::towlower);

    for (const auto& ext : SUSPICIOUS_EXTENSIONS) {
        if (lowerPath.length() >= ext.length()) {
            if (lowerPath.compare(lowerPath.length() - ext.length(), ext.length(), ext) == 0) {
                return true;
            }
        }
    }
    return false;
}

// Converts narrow character sequences safely over to wide representations
wstring AnsiToWide(const string& str) {
    if (str.empty()) return L"";
    int size_needed = MultiByteToWideChar(CP_ACP, 0, &str[0], (int)str.size(), NULL, 0);
    wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_ACP, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}


// --- API Hooks Implementation ---

// 1. Hook for MoveFileExW (Directly targets your new Unicode PoC application)
BOOL WINAPI myMoveFileExWHook(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    DWORD dwFlags)
{
    if (lpExistingFileName && lpNewFileName) {
        wstring newPath(lpNewFileName);

        if (IsSuspiciousExtension(newPath)) {
            wcout << L"\n[!] CYBER DECEPTION ALARM (MoveFileExW) [!]" << endl;
            wcout << L"[-] Intercepted Ransomware Modification Attempt!" << endl;
            wcout << L"[-] Source Target: " << lpExistingFileName << endl;
            wcout << L"[-] Modified Path: " << lpNewFileName << endl;
            cout << L"[+] DEFENSE STRATEGY: Executing FakeFailure routine..." << endl;

            // Enforce defense policy: block execution, fake a system failure, and report back to caller
            SetLastError(ERROR_ACCESS_DENIED);
            return FALSE;
        }
    }

    // Direct path execution for legitimate application file adjustments
    return MoveFileExW(lpExistingFileName, lpNewFileName, dwFlags);
}

// 2. Hook for MoveFileExA (Provides comprehensive safety coverage against alternate variants)
BOOL WINAPI myMoveFileExAHook(
    LPCSTR lpExistingFileName,
    LPCSTR lpNewFileName,
    DWORD dwFlags)
{
    if (lpExistingFileName && lpNewFileName) {
        wstring newPath = AnsiToWide(lpNewFileName);

        if (IsSuspiciousExtension(newPath)) {
            cout << "\n[!] CYBER DECEPTION ALARM (MoveFileExA) [!]" << endl;
            wcout << L"[-] Intercepted Ransomware Modification Attempt!" << endl;
            cout << L"[-] Source Target: " << lpExistingFileName << endl;
            cout << L"[-] Modified Path: " << lpNewFileName << endl;
            cout << L"[+] DEFENSE STRATEGY: Executing FakeFailure routine..." << endl;

            SetLastError(ERROR_ACCESS_DENIED);
            return FALSE;
        }
    }

    return MoveFileExA(lpExistingFileName, lpNewFileName, dwFlags);
}


// --- DLL Injection Entry Point ---
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
    cout << "\n[*] API Hook Deception Framework deployed successfully into target process." << endl;

    HOOK_TRACE_INFO hMoveFileExWHook = { NULL };
    HOOK_TRACE_INFO hMoveFileExAHook = { NULL };

    HMODULE hKernel32 = GetModuleHandle(TEXT("kernel32"));
    FARPROC moveFileExWAddr = GetProcAddress(hKernel32, "MoveFileExW");
    FARPROC moveFileExAAddr = GetProcAddress(hKernel32, "MoveFileExA");

    // Install runtime hooks using EasyHook engine
    NTSTATUS statusW = LhInstallHook(moveFileExWAddr, myMoveFileExWHook, nullptr, &hMoveFileExWHook);
    NTSTATUS statusA = LhInstallHook(moveFileExAAddr, myMoveFileExAHook, nullptr, &hMoveFileExAHook);

    if (FAILED(statusW) || FAILED(statusA)) {
        wstring errStr(RtlGetLastErrorString());
        wcerr << L"[-] Initialization Failure: Couldn't hook subsystem entry points: " << errStr << endl;
        return;
    }

    // Set configuration dynamically to intercept across all threads within the application domain
    ULONG ACLEntries[1] = { 0 };
    LhSetExclusiveACL(ACLEntries, 1, &hMoveFileExWHook);
    LhSetExclusiveACL(ACLEntries, 1, &hMoveFileExAHook);

    cout << "[*] Monitoring live. System watching for unauthorized extension changes." << endl;
}