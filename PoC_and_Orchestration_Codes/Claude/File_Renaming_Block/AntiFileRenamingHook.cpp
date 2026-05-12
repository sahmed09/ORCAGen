#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <easyhook.h>
#include <shlwapi.h>
#include <mutex>
#include <chrono>

#pragma comment(lib, "EasyHook32.lib")
#pragma comment(lib, "shlwapi.lib")

using namespace std;

// ==================== CONFIGURATION ====================
const int SUSPICIOUS_RENAME_THRESHOLD = 3;  // Block after N suspicious renames in time window
const int TIME_WINDOW_SECONDS = 5;          // Time window for counting suspicious activity
const int MAX_EXTENSION_LENGTH = 15;       // Max chars for suspicious extension check

// Suspicious extension patterns (lowercase)
unordered_set<wstring> SUSPICIOUS_EXTENSIONS = {
    L".locked", L".enc", L".encrypted", L".crypt", L".crypto",
    L".locky", L".cerber", L".revil", L".ryuk", L".wannacry",
    L".cryptolocker", L".cryptowall", L".teslacrypt", L".petya",
    L".maze", L".sodinokibi", L".dharma", L".phobos", L".stop"
};

// ==================== GLOBAL STATE ====================
struct RenameEvent {
    wstring oldPath;
    wstring newPath;
    chrono::steady_clock::time_point timestamp;
    bool wasSuspicious;
};

vector<RenameEvent> g_renameHistory;
mutex g_historyMutex;
int g_suspiciousRenameCount = 0;
bool g_ransomwareDetected = false;

// ==================== UTILITY FUNCTIONS ====================

// Convert string to lowercase
wstring ToLower(const wstring& str) {
    wstring result = str;
    transform(result.begin(), result.end(), result.begin(), ::towlower);
    return result;
}

// Extract file extension
wstring GetFileExtension(const wstring& path) {
    size_t dotPos = path.find_last_of(L".");
    if (dotPos != wstring::npos && dotPos < path.length() - 1) {
        wstring ext = path.substr(dotPos);
        if (ext.length() <= MAX_EXTENSION_LENGTH) {
            return ToLower(ext);
        }
    }
    return L"";
}

// Extract filename without path
wstring GetFileName(const wstring& path) {
    size_t slashPos = path.find_last_of(L"\\/");
    if (slashPos != wstring::npos) {
        return path.substr(slashPos + 1);
    }
    return path;
}

// Check if extension is suspicious
bool IsSuspiciousExtension(const wstring& path) {
    wstring ext = GetFileExtension(path);
    if (ext.empty()) return false;

    // Check against known suspicious extensions
    if (SUSPICIOUS_EXTENSIONS.find(ext) != SUSPICIOUS_EXTENSIONS.end()) {
        wcout << L"[DETECT] Known ransomware extension: " << ext << endl;
        return true;
    }

    return false;
}

// Check if the new filename contains base64-like encoded data
bool ContainsBase64Pattern(const wstring& filename) {
    // Check for pattern like: originalname.BASE64STRING.extension
    size_t firstDot = filename.find(L".");
    size_t lastDot = filename.find_last_of(L".");

    if (firstDot != wstring::npos && lastDot != wstring::npos && firstDot != lastDot) {
        // Extract middle part
        wstring middle = filename.substr(firstDot + 1, lastDot - firstDot - 1);

        // Check if it looks like base64 (contains +, /, =, or is long alphanumeric)
        if (middle.length() > 10) {
            bool hasBase64Chars = false;
            for (wchar_t c : middle) {
                if (c == L'+' || c == L'/' || c == L'=') {
                    hasBase64Chars = true;
                    break;
                }
            }

            if (hasBase64Chars || middle.length() > 20) {
                wcout << L"[DETECT] Base64-like pattern detected: " << middle << endl;
                return true;
            }
        }
    }

    return false;
}

// Check if the rename is adding a suspicious extension
bool IsAddingSuspiciousExtension(const wstring& oldPath, const wstring& newPath) {
    wstring oldExt = GetFileExtension(oldPath);
    wstring newExt = GetFileExtension(newPath);
    wstring oldFileName = GetFileName(oldPath);
    wstring newFileName = GetFileName(newPath);

    // Check 1: New extension is suspicious
    if (IsSuspiciousExtension(newPath)) {
        wcout << L"[DETECT] Suspicious extension in new path" << endl;
        return true;
    }

    // Check 2: Extension changed to something suspicious
    if (newExt != oldExt && !newExt.empty()) {
        // Original file had an extension, now it's being changed
        if (!oldExt.empty() && newExt != oldExt) {
            wcout << L"[DETECT] Extension change: " << oldExt << L" -> " << newExt << endl;
            return true;
        }
    }

    // Check 3: Base64 or encoded pattern in filename (ransomware victim ID)
    if (ContainsBase64Pattern(newFileName)) {
        return true;
    }

    // Check 4: Multiple extensions (file.txt.locked)
    size_t dotsInNew = count(newFileName.begin(), newFileName.end(), L'.');
    size_t dotsInOld = count(oldFileName.begin(), oldFileName.end(), L'.');

    if (dotsInNew > dotsInOld && dotsInNew >= 2) {
        wcout << L"[DETECT] Multiple extensions added (ransomware pattern)" << endl;
        return true;
    }

    return false;
}

// Count recent suspicious renames
int CountRecentSuspiciousRenames() {
    lock_guard<mutex> lock(g_historyMutex);
    auto now = chrono::steady_clock::now();
    int count = 0;

    for (const auto& event : g_renameHistory) {
        auto elapsed = chrono::duration_cast<chrono::seconds>(now - event.timestamp).count();
        if (elapsed <= TIME_WINDOW_SECONDS && event.wasSuspicious) {
            count++;
        }
    }

    return count;
}

// Log rename event
void LogRenameEvent(const wstring& oldPath, const wstring& newPath, bool suspicious, bool blocked) {
    lock_guard<mutex> lock(g_historyMutex);

    RenameEvent event;
    event.oldPath = oldPath;
    event.newPath = newPath;
    event.timestamp = chrono::steady_clock::now();
    event.wasSuspicious = suspicious;

    g_renameHistory.push_back(event);

    if (suspicious) {
        g_suspiciousRenameCount++;
    }

    // Cleanup old events (keep only last 100)
    if (g_renameHistory.size() > 100) {
        g_renameHistory.erase(g_renameHistory.begin());
    }

    // Log to console
    if (suspicious && blocked) {
        wcout << L"\n========================================" << endl;
        wcout << L"[!!!] BLOCKED RANSOMWARE-LIKE OPERATION" << endl;
        wcout << L"========================================" << endl;
        wcout << L"Old: " << oldPath << endl;
        wcout << L"New: " << newPath << endl;
        wcout << L"Suspicious count: " << g_suspiciousRenameCount << endl;
        wcout << L"========================================\n" << endl;
    }
    else if (suspicious && !blocked) {
        wcout << L"[WARNING] Suspicious rename detected but not blocked:" << endl;
        wcout << L"          Old: " << oldPath << endl;
        wcout << L"          New: " << newPath << endl;
    }
    else {
        wcout << L"[OK] Normal rename: " << GetFileName(oldPath) << L" -> " << GetFileName(newPath) << endl;
    }
}

// Decision engine: Should we block this rename?
bool ShouldBlockRename(const wstring& oldPath, const wstring& newPath) {

    // Check if adding suspicious extension
    if (!IsAddingSuspiciousExtension(oldPath, newPath)) {
        return false; // Not suspicious, allow
    }

    // Count recent suspicious activity
    int recentCount = CountRecentSuspiciousRenames();

    wcout << L"[ANALYSIS] Recent suspicious renames: " << recentCount << endl;

    // AGGRESSIVE BLOCKING: Block on first suspicious rename
    // This is the key change - don't wait for threshold
    wcout << L"[DECISION] Suspicious pattern detected - BLOCKING" << endl;
    g_ransomwareDetected = true;
    return true;

    /* Alternative: Threshold-based blocking
    if (recentCount >= SUSPICIOUS_RENAME_THRESHOLD) {
        wcout << L"[DECISION] Threshold exceeded - BLOCKING" << endl;
        g_ransomwareDetected = true;
        return true;
    }
    */
}

// ==================== HOOK FUNCTIONS ====================

// Hook: MoveFileW - PRIMARY TARGET FOR RANSOMWARE
BOOL WINAPI myMoveFileWHook(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName)
{
    if (!lpExistingFileName || !lpNewFileName) {
        return MoveFileW(lpExistingFileName, lpNewFileName);
    }

    wstring oldPath(lpExistingFileName);
    wstring newPath(lpNewFileName);

    wcout << L"\n[Hook] MoveFileW called:" << endl;
    wcout << L"       From: " << GetFileName(oldPath) << endl;
    wcout << L"       To:   " << GetFileName(newPath) << endl;

    bool shouldBlock = ShouldBlockRename(oldPath, newPath);
    bool isSuspicious = shouldBlock; // If we're blocking it, it's suspicious

    LogRenameEvent(oldPath, newPath, isSuspicious, shouldBlock);

    if (shouldBlock) {
        wcout << L"[ACTION] BLOCKING MoveFileW operation!" << endl;
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE; // Block the operation
    }

    wcout << L"[ACTION] Allowing MoveFileW operation" << endl;
    // Allow the operation
    BOOL result = MoveFileW(lpExistingFileName, lpNewFileName);

    if (!result) {
        DWORD error = GetLastError();
        wcout << L"[ERROR] MoveFileW failed with error: " << error << endl;
    }

    return result;
}

// Hook: MoveFileExW
BOOL WINAPI myMoveFileExWHook(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    DWORD dwFlags)
{
    if (!lpExistingFileName) {
        return MoveFileExW(lpExistingFileName, lpNewFileName, dwFlags);
    }

    wstring oldPath(lpExistingFileName);
    wstring newPath(lpNewFileName ? lpNewFileName : L"<DELETE>");

    wcout << L"\n[Hook] MoveFileExW called:" << endl;
    wcout << L"       From: " << GetFileName(oldPath) << endl;
    wcout << L"       To:   " << GetFileName(newPath) << endl;
    wcout << L"       Flags: 0x" << hex << dwFlags << dec << endl;

    if (lpNewFileName) {
        bool shouldBlock = ShouldBlockRename(oldPath, newPath);
        bool isSuspicious = shouldBlock;

        LogRenameEvent(oldPath, newPath, isSuspicious, shouldBlock);

        if (shouldBlock) {
            wcout << L"[ACTION] BLOCKING MoveFileExW operation!" << endl;
            SetLastError(ERROR_ACCESS_DENIED);
            return FALSE;
        }
    }

    wcout << L"[ACTION] Allowing MoveFileExW operation" << endl;
    BOOL result = MoveFileExW(lpExistingFileName, lpNewFileName, dwFlags);

    if (!result) {
        DWORD error = GetLastError();
        wcout << L"[ERROR] MoveFileExW failed with error: " << error << endl;
    }

    return result;
}

// Hook: CreateFileW (for monitoring)
HANDLE WINAPI myCreateFileWHook(
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile)
{
    wstring filePath(lpFileName);

    // Monitor writes to suspicious extensions
    if ((dwDesiredAccess & GENERIC_WRITE) && IsSuspiciousExtension(filePath)) {
        wcout << L"[Monitor] CreateFileW with suspicious extension: " << GetFileName(filePath) << endl;
    }

    return CreateFileW(lpFileName, dwDesiredAccess, dwShareMode,
        lpSecurityAttributes, dwCreationDisposition,
        dwFlagsAndAttributes, hTemplateFile);
}

// ==================== INJECTION ENTRY POINT ====================

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
    wcout << L"\n========================================" << endl;
    wcout << L"  RANSOMWARE DEFENSE HOOK ACTIVATED" << endl;
    wcout << L"========================================" << endl;
    wcout << L"Process ID: " << GetCurrentProcessId() << endl;
    wcout << L"Blocking Mode: ENABLED" << endl;
    wcout << L"Detection: AGGRESSIVE (Block on first suspicious rename)" << endl;
    wcout << L"========================================\n" << endl;

    HOOK_TRACE_INFO hMoveFileHook = { NULL };
    HOOK_TRACE_INFO hMoveFileExHook = { NULL };
    HOOK_TRACE_INFO hCreateFileHook = { NULL };

    // Get kernel32 module
    HMODULE kernel32 = GetModuleHandle(TEXT("kernel32"));
    if (!kernel32) {
        wcout << L"[FATAL] Failed to get kernel32 handle" << endl;
        return;
    }

    // Install MoveFileW hook
    FARPROC moveFileAddr = GetProcAddress(kernel32, "MoveFileW");
    if (moveFileAddr) {
        NTSTATUS status = LhInstallHook(moveFileAddr, myMoveFileWHook, nullptr, &hMoveFileHook);
        if (status == 0) {
            wcout << L"[SUCCESS] MoveFileW hook installed at: 0x" << hex << moveFileAddr << dec << endl;

            // Enable hook for all threads
            ULONG ACLEntries[1] = { 0 };
            LhSetExclusiveACL(ACLEntries, 1, &hMoveFileHook);
        }
        else {
            wcout << L"[FAILED] MoveFileW hook installation failed with status: " << status << endl;
        }
    }
    else {
        wcout << L"[FAILED] Could not find MoveFileW address" << endl;
    }

    // Install MoveFileExW hook
    FARPROC moveFileExAddr = GetProcAddress(kernel32, "MoveFileExW");
    if (moveFileExAddr) {
        NTSTATUS status = LhInstallHook(moveFileExAddr, myMoveFileExWHook, nullptr, &hMoveFileExHook);
        if (status == 0) {
            wcout << L"[SUCCESS] MoveFileExW hook installed at: 0x" << hex << moveFileExAddr << dec << endl;

            ULONG ACLEntries[1] = { 0 };
            LhSetExclusiveACL(ACLEntries, 1, &hMoveFileExHook);
        }
        else {
            wcout << L"[FAILED] MoveFileExW hook installation failed with status: " << status << endl;
        }
    }
    else {
        wcout << L"[FAILED] Could not find MoveFileExW address" << endl;
    }

    // Install CreateFileW hook (for monitoring)
    FARPROC createFileAddr = GetProcAddress(kernel32, "CreateFileW");
    if (createFileAddr) {
        NTSTATUS status = LhInstallHook(createFileAddr, myCreateFileWHook, nullptr, &hCreateFileHook);
        if (status == 0) {
            wcout << L"[SUCCESS] CreateFileW hook installed (monitoring mode)" << endl;

            ULONG ACLEntries[1] = { 0 };
            LhSetExclusiveACL(ACLEntries, 1, &hCreateFileHook);
        }
        else {
            wcout << L"[FAILED] CreateFileW hook installation failed with status: " << status << endl;
        }
    }

    wcout << L"\n[STATUS] All hooks activated - Monitoring started" << endl;
    wcout << L"[STATUS] Will block suspicious file rename operations" << endl;
    wcout << L"========================================\n" << endl;
}