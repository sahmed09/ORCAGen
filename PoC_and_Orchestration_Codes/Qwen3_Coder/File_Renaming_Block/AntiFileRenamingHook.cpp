#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <algorithm>
#include <vector>
#include <easyhook.h>

#pragma comment(lib, "EasyHook32.lib")

using namespace std;

// Patterns to block
const vector<wstring> suspiciousIndicators = {
    L".locked",
    L".enc",
    L".revil",
    L".cry",
    L".ransom",
    L".encrypted"
};

// Convert string to lowercase
wstring ToLower(const wstring& input) {
    wstring lower = input;
    transform(lower.begin(), lower.end(), lower.begin(), ::towlower);
    return lower;
}

bool IsSuspicious(const wstring& filename) {
    wstring lower = ToLower(filename);
    for (const auto& indicator : suspiciousIndicators) {
        if (lower.find(indicator) != wstring::npos) {
            return true;
        }
    }
    return false;
}

bool IsSuspiciousA(const string& filename) {
    string lower = filename;
    transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    for (const auto& indicatorW : suspiciousIndicators) {
        string indicator(indicatorW.begin(), indicatorW.end());
        if (lower.find(indicator) != string::npos) {
            return true;
        }
    }
    return false;
}

// ---------- Unicode Hooks ----------

BOOL WINAPI MyMoveFileW(LPCWSTR oldName, LPCWSTR newName) {
    wcout << L"[Hook] MoveFileW: " << oldName << L" -> " << newName << endl;
    if (IsSuspicious(newName)) {
        wcout << L"[BLOCKED] Rename blocked: " << newName << endl;
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }
    return MoveFileW(oldName, newName);
}

BOOL WINAPI MyMoveFileExW(LPCWSTR oldName, LPCWSTR newName, DWORD flags) {
    wcout << L"[Hook] MoveFileExW: " << oldName << L" -> " << newName << endl;
    if (IsSuspicious(newName)) {
        wcout << L"[BLOCKED] Rename blocked: " << newName << endl;
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }
    return MoveFileExW(oldName, newName, flags);
}

// ---------- ANSI Hooks ----------

BOOL WINAPI MyMoveFileA(LPCSTR oldName, LPCSTR newName) {
    cout << "[Hook] MoveFileA: " << oldName << " -> " << newName << endl;
    if (IsSuspiciousA(newName)) {
        cout << "[BLOCKED] Rename blocked: " << newName << endl;
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }
    return MoveFileA(oldName, newName);
}

BOOL WINAPI MyMoveFileExA(LPCSTR oldName, LPCSTR newName, DWORD flags) {
    cout << "[Hook] MoveFileExA: " << oldName << " -> " << newName << endl;
    if (IsSuspiciousA(newName)) {
        cout << "[BLOCKED] Rename blocked: " << newName << endl;
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }
    return MoveFileExA(oldName, newName, flags);
}

// ---------- Entry Point ----------

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
    cout << "[*] DLL Injected.\n";

    HOOK_TRACE_INFO hMoveW = {}, hMoveExW = {}, hMoveA = {}, hMoveExA = {};

    FARPROC pMoveW = GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "MoveFileW");
    FARPROC pMoveExW = GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "MoveFileExW");
    FARPROC pMoveA = GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "MoveFileA");
    FARPROC pMoveExA = GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "MoveFileExA");

    if (!pMoveW || !pMoveExW || !pMoveA || !pMoveExA) {
        cout << "[!] Failed to resolve API addresses." << endl;
        return;
    }

    // Install Hooks
    LhInstallHook(pMoveW, MyMoveFileW, nullptr, &hMoveW);
    LhInstallHook(pMoveExW, MyMoveFileExW, nullptr, &hMoveExW);
    LhInstallHook(pMoveA, MyMoveFileA, nullptr, &hMoveA);
    LhInstallHook(pMoveExA, MyMoveFileExA, nullptr, &hMoveExA);

    // Activate hooks for all threads
    ULONG ACLEntries[1] = { 0 };
    LhSetExclusiveACL(ACLEntries, 1, &hMoveW);
    LhSetExclusiveACL(ACLEntries, 1, &hMoveExW);
    LhSetExclusiveACL(ACLEntries, 1, &hMoveA);
    LhSetExclusiveACL(ACLEntries, 1, &hMoveExA);

    cout << "[*] File rename hooks installed and activated.\n";
}
