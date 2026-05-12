// AntiKeyloggerHook.cpp
#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <easyhook.h>
#include <random>

#pragma comment(lib, "EasyHook32.lib")

using namespace std;

// ============================================================================
// RANDOM GENERATOR FOR DECOY DATA
// ============================================================================
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> keyDist(0x41, 0x5A);  // A-Z range
std::uniform_int_distribution<> chanceDist(0, 99);

// ============================================================================
// HOOK FUNCTION PROTOTYPES
// ============================================================================
typedef HHOOK(WINAPI* SetWindowsHookEx_t)(int, HOOKPROC, HINSTANCE, DWORD);
typedef BOOL(WINAPI* WriteFile_t)(HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED);
typedef HANDLE(WINAPI* CreateFileW_t)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
typedef SHORT(WINAPI* GetAsyncKeyState_t)(int);
typedef SHORT(WINAPI* GetKeyState_t)(int);
typedef int(WINAPI* GetKeyNameTextA_t)(LONG, LPSTR, int);

// Original function pointers
SetWindowsHookEx_t Original_SetWindowsHookEx = SetWindowsHookEx;
WriteFile_t Original_WriteFile = WriteFile;
CreateFileW_t Original_CreateFileW = CreateFileW;
GetAsyncKeyState_t Original_GetAsyncKeyState = GetAsyncKeyState;
GetKeyState_t Original_GetKeyState = GetKeyState;
GetKeyNameTextA_t Original_GetKeyNameTextA = GetKeyNameTextA;

// Track if keylogger file handle
HANDLE g_keylogFileHandle = INVALID_HANDLE_VALUE;

// ============================================================================
// HOOKED FUNCTIONS
// ============================================================================

// Hook: SetWindowsHookEx - Detect and allow but mark for monitoring
HHOOK WINAPI mySetWindowsHookExHook(
    int idHook,
    HOOKPROC lpfn,
    HINSTANCE hmod,
    DWORD dwThreadId)
{
    if (idHook == WH_KEYBOARD_LL || idHook == WH_KEYBOARD)
    {
        cout << "[DEFENSE] Keyboard hook detected! Type: " 
             << (idHook == WH_KEYBOARD_LL ? "WH_KEYBOARD_LL" : "WH_KEYBOARD") 
             << endl;
    }

    // Allow the hook to be installed (we'll intercept data flow instead)
    return Original_SetWindowsHookEx(idHook, lpfn, hmod, dwThreadId);
}

// Hook: CreateFileW - Detect keylog file creation
HANDLE WINAPI myCreateFileWHook(
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile)
{
    if (lpFileName != nullptr)
    {
        std::wstring filename(lpFileName);
        
        // Check if this looks like a keylog file
        if (filename.find(L"keylog") != std::wstring::npos ||
            filename.find(L"keys.txt") != std::wstring::npos ||
            filename.find(L"log.txt") != std::wstring::npos)
        {
            wcout << L"[DEFENSE] Suspicious keylog file detected: " << lpFileName << endl;
            
            HANDLE hFile = Original_CreateFileW(
                lpFileName,
                dwDesiredAccess,
                dwShareMode,
                lpSecurityAttributes,
                dwCreationDisposition,
                dwFlagsAndAttributes,
                hTemplateFile);
            
            // Store handle for WriteFile interception
            if (hFile != INVALID_HANDLE_VALUE)
            {
                g_keylogFileHandle = hFile;
                cout << "[DEFENSE] Tracking keylog file handle: " << hFile << endl;
            }
            
            return hFile;
        }
    }

    return Original_CreateFileW(
        lpFileName,
        dwDesiredAccess,
        dwShareMode,
        lpSecurityAttributes,
        dwCreationDisposition,
        dwFlagsAndAttributes,
        hTemplateFile);
}

// Hook: WriteFile - Replace real keystrokes with decoy data
BOOL WINAPI myWriteFileHook(
    HANDLE hFile,
    LPCVOID lpBuffer,
    DWORD nNumberOfBytesToWrite,
    LPDWORD lpNumberOfBytesWritten,
    LPOVERLAPPED lpOverlapped)
{
    // If writing to the tracked keylog file
    if (hFile == g_keylogFileHandle && lpBuffer != nullptr && nNumberOfBytesToWrite > 0)
    {
        std::string originalData((const char*)lpBuffer, nNumberOfBytesToWrite);
        
        // Check if this looks like a key log entry
        if (originalData.find("[KEY]") != std::string::npos)
        {
            // Generate random decoy keystroke
            char decoyKey = (char)keyDist(gen);
            std::string decoyData = "[KEY] " + std::string(1, decoyKey) + "\r\n";
            
            cout << "[DECEPTION] Original: " << originalData.substr(0, originalData.find("\r\n")) 
                 << " -> Decoy: [KEY] " << decoyKey << endl;
            
            // Write decoy data instead
            return Original_WriteFile(
                hFile,
                decoyData.c_str(),
                (DWORD)decoyData.size(),
                lpNumberOfBytesWritten,
                lpOverlapped);
        }
    }

    // Normal WriteFile for everything else
    return Original_WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, 
                             lpNumberOfBytesWritten, lpOverlapped);
}

// Hook: GetAsyncKeyState - Return decoy state when real key pressed
SHORT WINAPI myGetAsyncKeyStateHook(int vKey)
{
    SHORT actualState = Original_GetAsyncKeyState(vKey);
    
    // If ANY key is actually pressed (high-order bit set)
    if (actualState & 0x8000)
    {
        // Generate a random decoy key
        int decoyKey = keyDist(gen);
        
        // Return "not pressed" for the actual key
        SHORT fakeState = actualState & 0x7FFF;  // Clear the pressed bit
        
        cout << "[DECEPTION] Real key VK:" << vKey 
             << " masked, decoy key VK:" << decoyKey << " injected" << endl;
        
        return fakeState;
    }
    
    // Also occasionally claim random keys are pressed to add noise
    if (chanceDist(gen) < 15)  // 15% chance
    {
        int decoyKey = keyDist(gen);
        if (decoyKey != vKey)
        {
            cout << "[DECEPTION] Injecting noise for VK:" << vKey << endl;
            return 0x8000;  // Simulate key pressed
        }
    }
    
    return actualState;
}

// Hook: GetKeyState - Similar deception for synchronous checks
SHORT WINAPI myGetKeyStateHook(int vKey)
{
    SHORT actualState = Original_GetKeyState(vKey);
    
    if (actualState & 0x8000)
    {
        int decoyKey = keyDist(gen);
        SHORT fakeState = actualState & 0x7FFF;
        
        cout << "[DECEPTION] GetKeyState - Real VK:" << vKey 
             << " masked, decoy VK:" << decoyKey << endl;
        
        return fakeState;
    }
    
    if (chanceDist(gen) < 10)
    {
        return 0x8000;
    }
    
    return actualState;
}

// Hook: GetKeyNameTextA - Return decoy key names
int WINAPI myGetKeyNameTextAHook(LONG lParam, LPSTR lpString, int cchSize)
{
    int result = Original_GetKeyNameTextA(lParam, lpString, cchSize);
    
    if (result > 0 && lpString != nullptr)
    {
        // Replace with random decoy character
        char decoyKey = (char)keyDist(gen);
        
        cout << "[DECEPTION] GetKeyNameTextA - Original: " << lpString 
             << " -> Decoy: " << decoyKey << endl;
        
        lpString[0] = decoyKey;
        lpString[1] = '\0';
        
        return 1;
    }
    
    return result;
}

// ============================================================================
// EASYHOOK ENTRY POINT
// ============================================================================
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(
    REMOTE_ENTRY_INFO* inRemoteInfo)
{
    cout << "========================================" << endl;
    cout << "[*] Anti-Keylogger Defense Activated" << endl;
    cout << "========================================" << endl;

    HOOK_TRACE_INFO hHookSetWindowsHookEx = { NULL };
    HOOK_TRACE_INFO hHookCreateFileW = { NULL };
    HOOK_TRACE_INFO hHookWriteFile = { NULL };
    HOOK_TRACE_INFO hHookGetAsyncKeyState = { NULL };
    HOOK_TRACE_INFO hHookGetKeyState = { NULL };
    HOOK_TRACE_INFO hHookGetKeyNameTextA = { NULL };

    NTSTATUS result;

    // Hook SetWindowsHookEx (user32.dll)
    FARPROC setHookAddr = GetProcAddress(GetModuleHandle(TEXT("user32")), "SetWindowsHookExA");
    if (setHookAddr)
    {
        result = LhInstallHook(setHookAddr, mySetWindowsHookExHook, nullptr, &hHookSetWindowsHookEx);
        if (SUCCEEDED(result))
            cout << "[+] SetWindowsHookExA hooked successfully" << endl;
        else
            cerr << "[-] Failed to hook SetWindowsHookExA" << endl;
    }

    // Hook CreateFileW (kernel32.dll)
    FARPROC createFileAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "CreateFileW");
    if (createFileAddr)
    {
        result = LhInstallHook(createFileAddr, myCreateFileWHook, nullptr, &hHookCreateFileW);
        if (SUCCEEDED(result))
            cout << "[+] CreateFileW hooked successfully" << endl;
        else
            cerr << "[-] Failed to hook CreateFileW" << endl;
    }

    // Hook WriteFile (kernel32.dll)
    FARPROC writeFileAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "WriteFile");
    if (writeFileAddr)
    {
        result = LhInstallHook(writeFileAddr, myWriteFileHook, nullptr, &hHookWriteFile);
        if (SUCCEEDED(result))
            cout << "[+] WriteFile hooked successfully" << endl;
        else
            cerr << "[-] Failed to hook WriteFile" << endl;
    }

    // Hook GetAsyncKeyState (user32.dll)
    FARPROC asyncKeyAddr = GetProcAddress(GetModuleHandle(TEXT("user32")), "GetAsyncKeyState");
    if (asyncKeyAddr)
    {
        result = LhInstallHook(asyncKeyAddr, myGetAsyncKeyStateHook, nullptr, &hHookGetAsyncKeyState);
        if (SUCCEEDED(result))
            cout << "[+] GetAsyncKeyState hooked successfully" << endl;
        else
            cerr << "[-] Failed to hook GetAsyncKeyState" << endl;
    }

    // Hook GetKeyState (user32.dll)
    FARPROC keyStateAddr = GetProcAddress(GetModuleHandle(TEXT("user32")), "GetKeyState");
    if (keyStateAddr)
    {
        result = LhInstallHook(keyStateAddr, myGetKeyStateHook, nullptr, &hHookGetKeyState);
        if (SUCCEEDED(result))
            cout << "[+] GetKeyState hooked successfully" << endl;
        else
            cerr << "[-] Failed to hook GetKeyState" << endl;
    }

    // Hook GetKeyNameTextA (user32.dll)
    FARPROC keyNameAddr = GetProcAddress(GetModuleHandle(TEXT("user32")), "GetKeyNameTextA");
    if (keyNameAddr)
    {
        result = LhInstallHook(keyNameAddr, myGetKeyNameTextAHook, nullptr, &hHookGetKeyNameTextA);
        if (SUCCEEDED(result))
            cout << "[+] GetKeyNameTextA hooked successfully" << endl;
        else
            cerr << "[-] Failed to hook GetKeyNameTextA" << endl;
    }

    // Enable all hooks for all threads (Exclusive ACL)
    ULONG ACLEntries[1] = { 0 };
    LhSetExclusiveACL(ACLEntries, 1, &hHookSetWindowsHookEx);
    LhSetExclusiveACL(ACLEntries, 1, &hHookCreateFileW);
    LhSetExclusiveACL(ACLEntries, 1, &hHookWriteFile);
    LhSetExclusiveACL(ACLEntries, 1, &hHookGetAsyncKeyState);
    LhSetExclusiveACL(ACLEntries, 1, &hHookGetKeyState);
    LhSetExclusiveACL(ACLEntries, 1, &hHookGetKeyNameTextA);

    cout << "[*] All hooks activated - Keylogger deception active!" << endl;
    cout << "========================================" << endl;
}