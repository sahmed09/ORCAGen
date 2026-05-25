#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <easyhook.h>

#pragma comment(lib, "EasyHook32.lib") // Swap to EasyHook64.lib if running an x64 build

using namespace std;

// --- Ransomware Signature Database ---
// List of known high-risk extensions we want to catch and block.
const vector<wstring> SUSPICIOUS_EXTENSIONS = {
    L".locked", L".enc", L".revil", L".encrypted",
    L".wannacry", L".crypto", L".locky", L".crypted"
};

// --- Helper Functions for String Evaluation ---
// Checks if a file path ends with any of our targeted extensions (Case-Insensitive)
bool IsSuspiciousExtension(const wstring& filePath) {
    if (filePath.empty()) return false;

    // Convert path to lowercase to bypass basic evasion attempts
    wstring lowerPath = filePath;
    transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::towlower);

    for (const auto& ext : SUSPICIOUS_EXTENSIONS) {
        if (lowerPath.length() >= ext.length()) {
            // Check if the tail of the string matches the extension
            if (lowerPath.compare(lowerPath.length() - ext.length(), ext.length(), ext) == 0) {
                return true;
            }
        }
    }
    return false;
}

// Inline converter for ANSI strings used in the 'A' variants of Win32 APIs
wstring AnsiToWide(const string& str) {
    if (str.empty()) return L"";
    int size_needed = MultiByteToWideChar(CP_ACP, 0, &str[0], (int)str.size(), NULL, 0);
    wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_ACP, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}


// --- API Function Pointer Types ---
// Required to cleanly call the original unhooked system APIs
typedef BOOL(WINAPI* MoveFileExA_p)(LPCSTR lpExistingFileName, LPCSTR lpNewFileName, DWORD dwFlags);
typedef BOOL(WINAPI* MoveFileExW_p)(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, DWORD dwFlags);


// --- Hook Implementations ---

// 1. Hook for MoveFileExA (Used by your Malware PoC)
BOOL WINAPI myMoveFileExAHook(LPCSTR lpExistingFileName, LPCSTR lpNewFileName, DWORD dwFlags) {
    if (lpExistingFileName && lpNewFileName) {
        wstring existingPath = AnsiToWide(lpExistingFileName);
        wstring newPath = AnsiToWide(lpNewFileName);

        // Analyze the target destination path
        if (IsSuspiciousExtension(newPath)) {
            cout << "\n[!] DECEPTION TRIGGERED (MoveFileExA) [!]" << endl;
            wcout << L"[-] Intercepted Ransomware Activity!" << endl;
            wcout << L"[-] Attempted: " << existingPath << L" -> " << newPath << endl;
            cout << L"[+] Blocking file system modification..." << endl;
            cout << L"[+] Feeding fake SUCCESS status back to the process." << endl;

            // Honeypot action: return TRUE to mimic successful encryption.
            // This prevents the ransomware from crashing or trying alternative destruction methods.
            return TRUE;
        }
    }

    // Pass-through: If it is a normal file rename operation, execute it normally
    return MoveFileExA(lpExistingFileName, lpNewFileName, dwFlags);
}

// 2. Hook for MoveFileExW (Catches native Wide/Unicode variants)
BOOL WINAPI myMoveFileExWHook(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, DWORD dwFlags) {
    if (lpExistingFileName && lpNewFileName) {
        wstring newPath(lpNewFileName);

        if (IsSuspiciousExtension(newPath)) {
            wcout << L"\n[!] DECEPTION TRIGGERED (MoveFileExW) [!]" << endl;
            wcout << L"[-] Intercepted Ransomware Activity!" << endl;
            wcout << L"[-] Attempted: " << lpExistingFileName << L" -> " << lpNewFileName << endl;
            cout << L"[+] Blocking file system modification..." << endl;
            cout << L"[+] Feeding fake SUCCESS status back to the process." << endl;

            return TRUE;
        }
    }

    return MoveFileExW(lpExistingFileName, lpNewFileName, dwFlags);
}


// --- Injection Entry Point ---
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
    cout << "\n[*] Cyber Deception Guard active inside target process." << endl;

    HOOK_TRACE_INFO hMoveFileExAHook = { NULL };
    HOOK_TRACE_INFO hMoveFileExWHook = { NULL };

    // Locate standard export addresses from Kernel32
    HMODULE hKernel32 = GetModuleHandle(TEXT("kernel32"));
    FARPROC moveFileExAAddr = GetProcAddress(hKernel32, "MoveFileExA");
    FARPROC moveFileExWAddr = GetProcAddress(hKernel32, "MoveFileExW");

    // Install the Inline Hooks via EasyHook
    NTSTATUS statusA = LhInstallHook(moveFileExAAddr, myMoveFileExAHook, nullptr, &hMoveFileExAHook);
    NTSTATUS statusW = LhInstallHook(moveFileExWAddr, myMoveFileExWHook, nullptr, &hMoveFileExWHook);

    if (FAILED(statusA) || FAILED(statusW)) {
        wstring errStr(RtlGetLastErrorString());
        wcerr << L"[-] Cyber Deception Module: Failed to initialize hooks: " << errStr << endl;
        return;
    }

    // Activate the interception rules globally for all threads inside this process
    ULONG ACLEntries[1] = { 0 };
    LhSetExclusiveACL(ACLEntries, 1, &hMoveFileExAHook);
    LhSetExclusiveACL(ACLEntries, 1, &hMoveFileExWHook);

    cout << "[*] System tracking active. Monitoring MoveFileEx variations." << endl;
}