#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <shlwapi.h>
#include <easyhook.h>
#include <mutex>
#include <chrono>
#include <fstream>

#pragma comment(lib, "EasyHook32.lib")
#pragma comment(lib, "shlwapi.lib")

using namespace std;

// ========== Configuration ==========
const int RENAME_THRESHOLD = 5;           // Max renames per minute
const int SUSPICIOUS_THRESHOLD = 3;        // Suspicious renames before strict blocking
const int TIME_WINDOW_MS = 60000;         // 1 minute window

// ========== Suspicious Extension Database ==========
unordered_set<wstring> suspiciousExtensions = {
    L".locked", L".encrypted", L".enc", L".crypt", L".crypto",
    L".REvil", L".wannacry", L".cerber", L".locky", L".petya",
    L".ryuk", L".sodinokibi", L".darkside", L".conti", L".maze",
    L".eking", L".dharma", L".phobos", L".stop", L".djvu"
};

// ========== Trusted Process Whitelist ==========
unordered_set<wstring> trustedProcesses = {
    L"explorer.exe", L"notepad.exe", L"winword.exe",
    L"excel.exe", L"powerpnt.exe", L"photoshop.exe",
    L"cmd.exe", L"powershell.exe"
};

// ========== Monitoring Structures ==========
struct RenameEvent {
    wstring oldPath;
    wstring newPath;
    chrono::steady_clock::time_point timestamp;
    bool wasSuspicious;
};

vector<RenameEvent> renameHistory;
mutex historyMutex;
int suspiciousAttempts = 0;
bool strictBlockingMode = false;

// ========== Logging ==========
ofstream logFile;
mutex logMutex;

void InitializeLog() {
    lock_guard<mutex> lock(logMutex);
    logFile.open("RansomwareDefense.log", ios::app);
    if (logFile.is_open()) {
        logFile << "\n=== Ransomware Defense System Started ===\n";
        logFile << "Timestamp: " << chrono::system_clock::now().time_since_epoch().count() << "\n\n";
    }
}

void WriteLog(const wstring& message) {
    lock_guard<mutex> lock(logMutex);
    if (logFile.is_open()) {
        logFile << "[" << chrono::system_clock::now().time_since_epoch().count() << "] ";
        wcout << message;
        // Convert to narrow string for file
        string narrowMsg(message.begin(), message.end());
        logFile << narrowMsg << endl;
    }
}

// ========== Utility Functions ==========

wstring GetFileExtension(const wstring& path) {
    size_t dotPos = path.find_last_of(L'.');
    if (dotPos != wstring::npos) {
        wstring ext = path.substr(dotPos);
        transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
        return ext;
    }
    return L"";
}

bool IsSuspiciousExtension(const wstring& extension) {
    wstring lowerExt = extension;
    transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(), ::towlower);
    return suspiciousExtensions.find(lowerExt) != suspiciousExtensions.end();
}

bool IsBase64Pattern(const wstring& filename) {
    // Check for base64-encoded segments (common in ransomware)
    size_t firstDot = filename.find(L'.');
    if (firstDot == wstring::npos) return false;

    size_t secondDot = filename.find(L'.', firstDot + 1);
    if (secondDot == wstring::npos) return false;

    wstring segment = filename.substr(firstDot + 1, secondDot - firstDot - 1);
    if (segment.length() < 8) return false;

    // Base64 characters check
    int base64Count = 0;
    for (wchar_t c : segment) {
        if ((c >= L'A' && c <= L'Z') || (c >= L'a' && c <= L'z') ||
            (c >= L'0' && c <= L'9') || c == L'+' || c == L'/' || c == L'=') {
            base64Count++;
        }
    }
    // If more than 80% are base64 chars, likely encoded
    return (base64Count * 100 / segment.length()) > 80;
}

bool IsExtensionChanged(const wstring& oldPath, const wstring& newPath) {
    wstring oldExt = GetFileExtension(oldPath);
    wstring newExt = GetFileExtension(newPath);
    return oldExt != newExt && !newExt.empty();
}

void CleanupOldEvents() {
    auto now = chrono::steady_clock::now();
    renameHistory.erase(
        remove_if(renameHistory.begin(), renameHistory.end(),
            [&now](const RenameEvent& event) {
                auto elapsed = chrono::duration_cast<chrono::milliseconds>(now - event.timestamp).count();
                return elapsed > TIME_WINDOW_MS;
            }),
        renameHistory.end()
                );
}

int GetRecentRenameCount() {
    lock_guard<mutex> lock(historyMutex);
    CleanupOldEvents();
    return renameHistory.size();
}

int GetSuspiciousRenameCount() {
    lock_guard<mutex> lock(historyMutex);
    CleanupOldEvents();
    return count_if(renameHistory.begin(), renameHistory.end(),
        [](const RenameEvent& e) { return e.wasSuspicious; });
}

bool IsTrustedProcess() {
    wchar_t processPath[MAX_PATH];
    GetModuleFileNameW(NULL, processPath, MAX_PATH);
    wstring processName = PathFindFileNameW(processPath);
    transform(processName.begin(), processName.end(), processName.begin(), ::towlower);
    return trustedProcesses.find(processName) != trustedProcesses.end();
}

void LogRenameAttempt(const wstring& oldPath, const wstring& newPath, bool suspicious, bool blocked, const wstring& action) {
    lock_guard<mutex> lock(historyMutex);

    wstring logMsg = L"\n========== RENAME DETECTED ==========\n";
    logMsg += L"[Old] " + oldPath + L"\n";
    logMsg += L"[New] " + newPath + L"\n";
    logMsg += L"[Suspicious] " + wstring(suspicious ? L"YES" : L"NO") + L"\n";
    logMsg += L"[Action] " + action + L"\n";
    logMsg += L"[Recent Renames] " + to_wstring(renameHistory.size()) + L"\n";
    logMsg += L"[Suspicious Count] " + to_wstring(suspiciousAttempts) + L"\n";
    logMsg += L"=====================================\n";

    WriteLog(logMsg);

    RenameEvent event;
    event.oldPath = oldPath;
    event.newPath = newPath;
    event.timestamp = chrono::steady_clock::now();
    event.wasSuspicious = suspicious;
    renameHistory.push_back(event);
}

bool AnalyzeRenamePattern(const wstring& oldPath, const wstring& newPath) {
    bool suspicious = false;
    wstring reasons;

    // Check 1: Suspicious extension
    wstring newExt = GetFileExtension(newPath);
    if (IsSuspiciousExtension(newExt)) {
        reasons += L"[!] Suspicious extension detected: " + newExt + L"\n";
        suspicious = true;
    }

    // Check 2: Extension change with Base64 pattern
    if (IsExtensionChanged(oldPath, newPath)) {
        reasons += L"[!] Extension changed\n";

        if (IsBase64Pattern(newPath)) {
            reasons += L"[!] Base64 pattern detected (victim ID encoding)\n";
            suspicious = true;
        }
    }

    // Check 3: Rapid rename pattern
    if (GetRecentRenameCount() > RENAME_THRESHOLD) {
        reasons += L"[!] High rename frequency: " + to_wstring(GetRecentRenameCount()) + L" renames in 60s\n";
        suspicious = true;
    }

    // Check 4: Multiple suspicious renames
    if (GetSuspiciousRenameCount() >= SUSPICIOUS_THRESHOLD) {
        reasons += L"[!] Multiple suspicious renames detected - ENTERING STRICT MODE\n";
        strictBlockingMode = true;
        suspicious = true;
    }

    if (!reasons.empty()) {
        WriteLog(reasons);
    }

    return suspicious;
}

// ========== Hook Functions with FakeFailure Deception ==========

BOOL WINAPI myMoveFileWHook(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName)
{
    bool isSuspicious = false;
    bool shouldBlock = false;
    wstring action = L"ALLOWED";

    // Analyze the rename operation
    if (lpExistingFileName && lpNewFileName) {
        isSuspicious = AnalyzeRenamePattern(lpExistingFileName, lpNewFileName);

        // Decision logic: Block suspicious operations from untrusted processes
        if (isSuspicious) {
            suspiciousAttempts++;

            // Block if suspicious and not from trusted process
            if (!IsTrustedProcess() || strictBlockingMode) {
                shouldBlock = true;
                action = L"BLOCKED (FakeFailure - File NOT renamed, malware deceived)";
            }
        }

        LogRenameAttempt(lpExistingFileName, lpNewFileName, isSuspicious, shouldBlock, action);
    }

    // === CRITICAL: FakeFailure Deception Strategy ===
    // DO NOT perform the actual rename, but return FALSE to deceive malware
    if (shouldBlock) {
        WriteLog(L"[DEFENSE] FakeFailure activated - Blocking rename and returning FALSE\n");
        WriteLog(L"[DEFENSE] Malware will receive ERROR_ACCESS_DENIED\n");
        WriteLog(L"[DEFENSE] Original file preserved at: " + wstring(lpExistingFileName) + L"\n");

        // Return FALSE (failure) to the malware
        // Set appropriate error code
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    // Allow legitimate operations - call original API
    BOOL result = MoveFileW(lpExistingFileName, lpNewFileName);

    if (!result) {
        WriteLog(L"[INFO] Legitimate rename failed (real system error)\n");
    }

    return result;
}

BOOL WINAPI myMoveFileExWHook(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    DWORD dwFlags)
{
    bool isSuspicious = false;
    bool shouldBlock = false;
    wstring action = L"ALLOWED";

    // Analyze the rename operation
    if (lpExistingFileName && lpNewFileName) {
        isSuspicious = AnalyzeRenamePattern(lpExistingFileName, lpNewFileName);

        // Decision logic
        if (isSuspicious) {
            suspiciousAttempts++;

            if (!IsTrustedProcess() || strictBlockingMode) {
                shouldBlock = true;
                action = L"BLOCKED (FakeFailure - File NOT renamed, malware deceived)";
            }
        }

        LogRenameAttempt(lpExistingFileName, lpNewFileName, isSuspicious, shouldBlock, action);
    }

    // === FakeFailure Deception ===
    if (shouldBlock) {
        WriteLog(L"[DEFENSE] FakeFailure activated (MoveFileEx) - Blocking and returning FALSE\n");
        WriteLog(L"[DEFENSE] Malware will receive ERROR_ACCESS_DENIED\n");
        WriteLog(L"[DEFENSE] Original file preserved at: " + wstring(lpExistingFileName) + L"\n");

        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    // Allow legitimate operations
    BOOL result = MoveFileExW(lpExistingFileName, lpNewFileName, dwFlags);

    if (!result) {
        WriteLog(L"[INFO] Legitimate rename failed (real system error)\n");
    }

    return result;
}

// Monitor file creation for additional intelligence
HANDLE WINAPI myCreateFileWHook(
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile)
{
    if (lpFileName) {
        wstring ext = GetFileExtension(lpFileName);
        if (IsSuspiciousExtension(ext)) {
            WriteLog(L"[WARNING] Suspicious file creation detected: " + wstring(lpFileName) + L"\n");
        }
    }

    return CreateFileW(lpFileName, dwDesiredAccess, dwShareMode,
        lpSecurityAttributes, dwCreationDisposition,
        dwFlagsAndAttributes, hTemplateFile);
}

// ========== DLL Entry Point ==========

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
    InitializeLog();

    wcout << L"\n";
    wcout << L"╔═══════════════════════════════════════════════════════════╗\n";
    wcout << L"║  RANSOMWARE DEFENSE SYSTEM - FakeFailure Mode             ║\n";
    wcout << L"║  Strategy: Block renames + Return FALSE to deceive        ║\n";
    wcout << L"║  Result: Malware thinks rename failed, files protected    ║\n";
    wcout << L"╚═══════════════════════════════════════════════════════════╝\n";
    wcout << L"\n";

    HOOK_TRACE_INFO hMoveFileHook = { NULL };
    HOOK_TRACE_INFO hMoveFileExHook = { NULL };
    HOOK_TRACE_INFO hCreateFileHook = { NULL };

    // Get API addresses
    HMODULE kernel32 = GetModuleHandle(TEXT("kernel32"));

    FARPROC moveFileAddr = GetProcAddress(kernel32, "MoveFileW");
    FARPROC moveFileExAddr = GetProcAddress(kernel32, "MoveFileExW");
    FARPROC createFileAddr = GetProcAddress(kernel32, "CreateFileW");

    // Install hooks
    NTSTATUS status;

    status = LhInstallHook(moveFileAddr, myMoveFileWHook, nullptr, &hMoveFileHook);
    if (status == 0) {
        wcout << L"[✓] MoveFileW hook installed (FakeFailure ready)\n";
        WriteLog(L"[✓] MoveFileW hook installed\n");
    }
    else {
        wcout << L"[✗] MoveFileW hook failed: " << status << L"\n";
    }

    status = LhInstallHook(moveFileExAddr, myMoveFileExWHook, nullptr, &hMoveFileExHook);
    if (status == 0) {
        wcout << L"[✓] MoveFileExW hook installed (FakeFailure ready)\n";
        WriteLog(L"[✓] MoveFileExW hook installed\n");
    }
    else {
        wcout << L"[✗] MoveFileExW hook failed: " << status << L"\n";
    }

    status = LhInstallHook(createFileAddr, myCreateFileWHook, nullptr, &hCreateFileHook);
    if (status == 0) {
        wcout << L"[✓] CreateFileW hook installed (monitoring)\n";
        WriteLog(L"[✓] CreateFileW hook installed\n");
    }
    else {
        wcout << L"[✗] CreateFileW hook failed: " << status << L"\n";
    }

    // Enable hooks for all threads
    ULONG ACLEntries[1] = { 0 };
    LhSetExclusiveACL(ACLEntries, 1, &hMoveFileHook);
    LhSetExclusiveACL(ACLEntries, 1, &hMoveFileExHook);
    LhSetExclusiveACL(ACLEntries, 1, &hCreateFileHook);

    wcout << L"\n[*] FakeFailure Defense Active\n";
    wcout << L"[*] Monitoring: MoveFileW, MoveFileExW, CreateFileW\n";
    wcout << L"[*] Strategy: Block + Return FALSE on suspicious renames\n";
    wcout << L"[*] Log file: C:\\RansomwareDefense.log\n\n";

    WriteLog(L"[*] Defense system fully operational\n\n");
}