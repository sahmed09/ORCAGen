#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <shlwapi.h>
#include <easyhook.h>
#include <fstream>
#include <ctime>
#include <mutex>

#pragma comment(lib, "EasyHook32.lib")
#pragma comment(lib, "shlwapi.lib")

using namespace std;

// ============================================================================
// CONFIGURATION & CONSTANTS
// ============================================================================

const int RAPID_RENAME_THRESHOLD = 10;  // Number of renames
const int RAPID_RENAME_WINDOW_MS = 2000; // Time window in milliseconds
const int MAX_HISTORY = 100;

// Suspicious ransomware extensions
const vector<wstring> SUSPICIOUS_EXTENSIONS = {
    L".locked", L".enc", L".encrypted", L".crypto", L".crypted",
    L".REvil", L".sodinokibi", L".ryuk", L".lockbit", L".conti",
    L".maze", L".eking", L".dharma", L".phobos", L".cerber",
    L".locky", L".wannacry", L".petya", L".badrabbit", L".gandcrab",
    L".crypt", L".cryptolocker", L".cryptowall", L".ransom",
    L".zzzzz", L".micro", L".vault", L".AES256", L".encrypt"
};

// Trusted processes (whitelist)
const vector<wstring> TRUSTED_PROCESSES = {
    L"explorer.exe",
    L"notepad.exe",
    L"winword.exe",
    L"excel.exe",
    L"powerpnt.exe",
    L"devenv.exe"
};

// ============================================================================
// GLOBAL STATE & SYNCHRONIZATION
// ============================================================================

struct RenameEvent {
    wstring oldPath;
    wstring newPath;
    DWORD timestamp;
    DWORD processId;
};

vector<RenameEvent> g_renameHistory;
mutex g_historyMutex;
wofstream g_logFile;
mutex g_logMutex;

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

wstring ToLowerCase(const wstring& str) {
    wstring result = str;
    transform(result.begin(), result.end(), result.begin(), ::towlower);
    return result;
}

wstring GetFileExtension(const wstring& path) {
    size_t dotPos = path.find_last_of(L'.');
    if (dotPos != wstring::npos && dotPos < path.length() - 1) {
        return ToLowerCase(path.substr(dotPos));
    }
    return L"";
}

wstring GetProcessName() {
    wchar_t processPath[MAX_PATH];
    if (GetModuleFileNameW(NULL, processPath, MAX_PATH)) {
        wstring fullPath(processPath);
        size_t lastSlash = fullPath.find_last_of(L"\\/");
        if (lastSlash != wstring::npos) {
            return fullPath.substr(lastSlash + 1);
        }
        return fullPath;
    }
    return L"unknown";
}

bool IsProcessTrusted() {
    wstring processName = ToLowerCase(GetProcessName());
    for (const auto& trusted : TRUSTED_PROCESSES) {
        if (processName == ToLowerCase(trusted)) {
            return true;
        }
    }
    return false;
}

bool HasSuspiciousExtension(const wstring& path) {
    wstring ext = GetFileExtension(path);
    for (const auto& suspExt : SUSPICIOUS_EXTENSIONS) {
        if (ext == ToLowerCase(suspExt)) {
            return true;
        }
    }
    return false;
}

bool HasBase64Pattern(const wstring& filename) {
    // Check for base64-like patterns (alphanumeric + equals)
    int base64Chars = 0;
    int totalChars = 0;

    for (wchar_t c : filename) {
        if (isalnum(c) || c == L'+' || c == L'/' || c == L'=') {
            base64Chars++;
        }
        totalChars++;
    }

    // If more than 80% of chars are base64-like, it's suspicious
    return totalChars > 10 && (base64Chars * 100 / totalChars) > 80;
}

bool IsMultipleExtensionChange(const wstring& oldPath, const wstring& newPath) {
    // Check if file has multiple extensions (e.g., file.txt.locked)
    wstring newFileName = newPath.substr(newPath.find_last_of(L"\\/") + 1);
    int dotCount = 0;
    for (wchar_t c : newFileName) {
        if (c == L'.') dotCount++;
    }
    return dotCount >= 2;
}

bool IsRapidRenaming() {
    lock_guard<mutex> lock(g_historyMutex);

    if (g_renameHistory.size() < RAPID_RENAME_THRESHOLD) {
        return false;
    }

    DWORD currentTime = GetTickCount();
    int recentCount = 0;

    for (auto it = g_renameHistory.rbegin();
        it != g_renameHistory.rend() && recentCount < RAPID_RENAME_THRESHOLD;
        ++it) {
        if (currentTime - it->timestamp < RAPID_RENAME_WINDOW_MS) {
            recentCount++;
        }
    }

    return recentCount >= RAPID_RENAME_THRESHOLD;
}

void LogEvent(const wstring& message, bool isBlocked = false) {
    lock_guard<mutex> lock(g_logMutex);

    if (!g_logFile.is_open()) {
        g_logFile.open(L"C:\\ransomware_defense_log.txt", ios::app);
    }

    if (g_logFile.is_open()) {
        time_t now = time(nullptr);
        wchar_t timeStr[26];
        _wctime_s(timeStr, 26, &now);
        timeStr[wcslen(timeStr) - 1] = L'\0'; // Remove newline

        g_logFile << L"[" << timeStr << L"] ";
        if (isBlocked) {
            g_logFile << L"[BLOCKED] ";
        }
        g_logFile << message << endl;
        g_logFile.flush();
    }

    wcout << (isBlocked ? L"[BLOCKED] " : L"[INFO] ") << message << endl;
}

void AddToHistory(const wstring& oldPath, const wstring& newPath) {
    lock_guard<mutex> lock(g_historyMutex);

    RenameEvent event;
    event.oldPath = oldPath;
    event.newPath = newPath;
    event.timestamp = GetTickCount();
    event.processId = GetCurrentProcessId();

    g_renameHistory.push_back(event);

    // Maintain history size
    if (g_renameHistory.size() > MAX_HISTORY) {
        g_renameHistory.erase(g_renameHistory.begin());
    }
}

// ============================================================================
// THREAT ANALYSIS
// ============================================================================

enum ThreatLevel {
    THREAT_NONE = 0,
    THREAT_LOW = 1,
    THREAT_MEDIUM = 2,
    THREAT_HIGH = 3,
    THREAT_CRITICAL = 4
};

ThreatLevel AnalyzeThreat(const wstring& oldPath, const wstring& newPath) {
    int threatScore = 0;

    // Check 1: Suspicious extension
    if (HasSuspiciousExtension(newPath)) {
        threatScore += 3;
    }

    // Check 2: Multiple extensions
    if (IsMultipleExtensionChange(oldPath, newPath)) {
        threatScore += 2;
    }

    // Check 3: Base64 pattern in filename
    wstring newFileName = newPath.substr(newPath.find_last_of(L"\\/") + 1);
    if (HasBase64Pattern(newFileName)) {
        threatScore += 2;
    }

    // Check 4: Rapid renaming behavior
    if (IsRapidRenaming()) {
        threatScore += 3;
    }

    // Check 5: Extension completely changed (not appended)
    wstring oldExt = GetFileExtension(oldPath);
    wstring newExt = GetFileExtension(newPath);
    if (!oldExt.empty() && !newExt.empty() && oldExt != newExt) {
        if (newPath.find(oldExt) == wstring::npos) {
            threatScore += 1;
        }
    }

    // Reduce score if trusted process
    if (IsProcessTrusted()) {
        threatScore = max(0, threatScore - 2);
    }

    // Map score to threat level
    if (threatScore >= 6) return THREAT_CRITICAL;
    if (threatScore >= 4) return THREAT_HIGH;
    if (threatScore >= 2) return THREAT_MEDIUM;
    if (threatScore >= 1) return THREAT_LOW;
    return THREAT_NONE;
}

// ============================================================================
// HOOKED FUNCTIONS
// ============================================================================

BOOL WINAPI MyMoveFileW(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName)
{
    wstring oldPath(lpExistingFileName ? lpExistingFileName : L"");
    wstring newPath(lpNewFileName ? lpNewFileName : L"");

    // Analyze threat level
    ThreatLevel threat = AnalyzeThreat(oldPath, newPath);

    // Log and decide
    if (threat >= THREAT_HIGH) {
        wstring logMsg = L"MoveFileW: " + oldPath + L" -> " + newPath +
            L" [Process: " + GetProcessName() + L"]";
        LogEvent(logMsg, true);

        AddToHistory(oldPath, newPath);

        // Return fake success to deceive ransomware
        SetLastError(ERROR_SUCCESS);
        return TRUE;
    }
    else if (threat >= THREAT_MEDIUM) {
        wstring logMsg = L"[SUSPICIOUS] MoveFileW: " + oldPath + L" -> " + newPath;
        LogEvent(logMsg, false);
    }

    // Allow legitimate operation
    AddToHistory(oldPath, newPath);
    return MoveFileW(lpExistingFileName, lpNewFileName);
}

BOOL WINAPI MyMoveFileExW(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    DWORD dwFlags)
{
    wstring oldPath(lpExistingFileName ? lpExistingFileName : L"");
    wstring newPath(lpNewFileName ? lpNewFileName : L"");

    ThreatLevel threat = AnalyzeThreat(oldPath, newPath);

    if (threat >= THREAT_HIGH) {
        wstring logMsg = L"MoveFileExW: " + oldPath + L" -> " + newPath +
            L" [Process: " + GetProcessName() + L"]";
        LogEvent(logMsg, true);

        AddToHistory(oldPath, newPath);

        SetLastError(ERROR_SUCCESS);
        return TRUE;
    }
    else if (threat >= THREAT_MEDIUM) {
        wstring logMsg = L"[SUSPICIOUS] MoveFileExW: " + oldPath + L" -> " + newPath;
        LogEvent(logMsg, false);
    }

    AddToHistory(oldPath, newPath);
    return MoveFileExW(lpExistingFileName, lpNewFileName, dwFlags);
}

BOOL WINAPI MyMoveFileWithProgressW(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    LPPROGRESS_ROUTINE lpProgressRoutine,
    LPVOID lpData,
    DWORD dwFlags)
{
    wstring oldPath(lpExistingFileName ? lpExistingFileName : L"");
    wstring newPath(lpNewFileName ? lpNewFileName : L"");

    ThreatLevel threat = AnalyzeThreat(oldPath, newPath);

    if (threat >= THREAT_HIGH) {
        wstring logMsg = L"MoveFileWithProgressW: " + oldPath + L" -> " + newPath;
        LogEvent(logMsg, true);

        AddToHistory(oldPath, newPath);

        SetLastError(ERROR_SUCCESS);
        return TRUE;
    }

    AddToHistory(oldPath, newPath);
    return MoveFileWithProgressW(lpExistingFileName, lpNewFileName,
        lpProgressRoutine, lpData, dwFlags);
}

// ============================================================================
// EASYHOOK ENTRY POINT
// ============================================================================

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
    wcout << L"[*] Ransomware Defense Hook Injection Started" << endl;
    wcout << L"[*] Target Process: " << GetProcessName() << endl;
    wcout << L"[*] Process ID: " << GetCurrentProcessId() << endl;

    LogEvent(L"Ransomware Defense System Initialized");

    HOOK_TRACE_INFO hMoveFileW = { NULL };
    HOOK_TRACE_INFO hMoveFileExW = { NULL };
    HOOK_TRACE_INFO hMoveFileWithProgressW = { NULL };

    // Install hooks
    NTSTATUS status;

    // Hook MoveFileW
    FARPROC moveFileWAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "MoveFileW");
    if (moveFileWAddr) {
        status = LhInstallHook(moveFileWAddr, MyMoveFileW, nullptr, &hMoveFileW);
        if (status == 0) {
            wcout << L"[+] MoveFileW hook installed successfully" << endl;
        }
        else {
            wcout << L"[-] Failed to install MoveFileW hook: " << status << endl;
        }
    }

    // Hook MoveFileExW
    FARPROC moveFileExWAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "MoveFileExW");
    if (moveFileExWAddr) {
        status = LhInstallHook(moveFileExWAddr, MyMoveFileExW, nullptr, &hMoveFileExW);
        if (status == 0) {
            wcout << L"[+] MoveFileExW hook installed successfully" << endl;
        }
        else {
            wcout << L"[-] Failed to install MoveFileExW hook: " << status << endl;
        }
    }

    // Hook MoveFileWithProgressW
    FARPROC moveFileProgressAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "MoveFileWithProgressW");
    if (moveFileProgressAddr) {
        status = LhInstallHook(moveFileProgressAddr, MyMoveFileWithProgressW, nullptr, &hMoveFileWithProgressW);
        if (status == 0) {
            wcout << L"[+] MoveFileWithProgressW hook installed successfully" << endl;
        }
        else {
            wcout << L"[-] Failed to install MoveFileWithProgressW hook: " << status << endl;
        }
    }

    // Enable hooks for all threads
    ULONG ACLEntries[1] = { 0 };
    LhSetExclusiveACL(ACLEntries, 1, &hMoveFileW);
    LhSetExclusiveACL(ACLEntries, 1, &hMoveFileExW);
    LhSetExclusiveACL(ACLEntries, 1, &hMoveFileWithProgressW);

    wcout << L"[*] All hooks activated - Monitoring file renames" << endl;
    LogEvent(L"All hooks activated successfully");

    // Keep DLL loaded - Wait to allow hooks to be established
    Sleep(5000);
}