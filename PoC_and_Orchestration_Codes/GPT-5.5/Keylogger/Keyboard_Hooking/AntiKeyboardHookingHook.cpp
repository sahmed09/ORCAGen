#include "pch.h"
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <iostream>
#include <easyhook.h>
#include <random>
#include <vector>

#pragma comment(lib, "EasyHook32.lib") // Use EasyHook64.lib for x64

using namespace std;

static HOOK_TRACE_INFO hGetAsyncKeyStateHook = { NULL };
static HOOK_TRACE_INFO hGetKeyStateHook = { NULL };
static HOOK_TRACE_INFO hSetWindowsHookExWHook = { NULL };
static HOOK_TRACE_INFO hSetWindowsHookExAHook = { NULL };

static std::random_device rd;
static std::mt19937 gen(rd());

static std::vector<int> g_DecoyKeys = {
    'A','B','C','D','E','F','G','H','I','J','K','L','M',
    'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
    '0','1','2','3','4','5','6','7','8','9',
    VK_SPACE, VK_RETURN, VK_TAB
};

static int g_CurrentDecoyKey = 'A';
static ULONGLONG g_LastRealKeyTick = 0;
static const DWORD DECOY_WINDOW_MS = 40;

int PickRandomDecoyKey(int realKey)
{
    std::uniform_int_distribution<> dist(0, (int)g_DecoyKeys.size() - 1);

    int decoy = realKey;
    while (decoy == realKey)
    {
        decoy = g_DecoyKeys[dist(gen)];
    }

    return decoy;
}

SHORT WINAPI myGetAsyncKeyStateHook(int vKey)
{
    SHORT actualState = GetAsyncKeyState(vKey);
    bool realPressed = (actualState & 0x8000) != 0;

    ULONGLONG now = GetTickCount64();

    if (realPressed)
    {
        g_CurrentDecoyKey = PickRandomDecoyKey(vKey);
        g_LastRealKeyTick = now;

        cout << "[Deception] Real VK " << vKey
            << " suppressed, decoy VK "
            << g_CurrentDecoyKey << " activated." << endl;

        return 0;
    }

    if ((now - g_LastRealKeyTick) <= DECOY_WINDOW_MS)
    {
        if (vKey == g_CurrentDecoyKey)
        {
            return 0x8001; // high bit = pressed, low bit = recently pressed
        }
    }

    return 0;
}

SHORT WINAPI myGetKeyStateHook(int nVirtKey)
{
    SHORT actualState = GetKeyState(nVirtKey);
    bool realPressed = (actualState & 0x8000) != 0;

    ULONGLONG now = GetTickCount64();

    if (realPressed)
    {
        g_CurrentDecoyKey = PickRandomDecoyKey(nVirtKey);
        g_LastRealKeyTick = now;

        cout << "[Deception] GetKeyState real VK "
            << nVirtKey << " replaced with decoy VK "
            << g_CurrentDecoyKey << endl;

        return 0;
    }

    if ((now - g_LastRealKeyTick) <= DECOY_WINDOW_MS)
    {
        if (nVirtKey == g_CurrentDecoyKey)
        {
            return 0x8000;
        }
    }

    return 0;
}

HHOOK WINAPI mySetWindowsHookExWHook(
    int idHook,
    HOOKPROC lpfn,
    HINSTANCE hmod,
    DWORD dwThreadId)
{
    if (idHook == WH_KEYBOARD || idHook == WH_KEYBOARD_LL)
    {
        cout << "[Blocked] Malware attempted SetWindowsHookExW keyboard hook." << endl;
        SetLastError(ERROR_ACCESS_DENIED);
        return NULL;
    }

    return SetWindowsHookExW(idHook, lpfn, hmod, dwThreadId);
}

HHOOK WINAPI mySetWindowsHookExAHook(
    int idHook,
    HOOKPROC lpfn,
    HINSTANCE hmod,
    DWORD dwThreadId)
{
    if (idHook == WH_KEYBOARD || idHook == WH_KEYBOARD_LL)
    {
        cout << "[Blocked] Malware attempted SetWindowsHookExA keyboard hook." << endl;
        SetLastError(ERROR_ACCESS_DENIED);
        return NULL;
    }

    return SetWindowsHookExA(idHook, lpfn, hmod, dwThreadId);
}

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(
    REMOTE_ENTRY_INFO * inRemoteInfo)
{
    cout << "[*] Keyboard deception DLL injected." << endl;

    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (!user32)
    {
        cout << "[-] Failed to get user32.dll handle." << endl;
        return;
    }

    FARPROC getAsyncAddr = GetProcAddress(user32, "GetAsyncKeyState");
    FARPROC getKeyStateAddr = GetProcAddress(user32, "GetKeyState");
    FARPROC setHookWAddr = GetProcAddress(user32, "SetWindowsHookExW");
    FARPROC setHookAAddr = GetProcAddress(user32, "SetWindowsHookExA");

    NTSTATUS result;

    result = LhInstallHook(
        getAsyncAddr,
        myGetAsyncKeyStateHook,
        nullptr,
        &hGetAsyncKeyStateHook
    );

    if (FAILED(result))
    {
        wcout << L"[-] Failed to hook GetAsyncKeyState: "
            << RtlGetLastErrorString() << endl;
    }
    else
    {
        cout << "[+] GetAsyncKeyState hook installed." << endl;
    }

    result = LhInstallHook(
        getKeyStateAddr,
        myGetKeyStateHook,
        nullptr,
        &hGetKeyStateHook
    );

    if (FAILED(result))
    {
        wcout << L"[-] Failed to hook GetKeyState: "
            << RtlGetLastErrorString() << endl;
    }
    else
    {
        cout << "[+] GetKeyState hook installed." << endl;
    }

    result = LhInstallHook(
        setHookWAddr,
        mySetWindowsHookExWHook,
        nullptr,
        &hSetWindowsHookExWHook
    );

    if (SUCCEEDED(result))
    {
        cout << "[+] SetWindowsHookExW hook installed." << endl;
    }

    result = LhInstallHook(
        setHookAAddr,
        mySetWindowsHookExAHook,
        nullptr,
        &hSetWindowsHookExAHook
    );

    if (SUCCEEDED(result))
    {
        cout << "[+] SetWindowsHookExA hook installed." << endl;
    }

    ULONG ACLEntries[1] = { 0 };

    LhSetExclusiveACL(ACLEntries, 1, &hGetAsyncKeyStateHook);
    LhSetExclusiveACL(ACLEntries, 1, &hGetKeyStateHook);
    LhSetExclusiveACL(ACLEntries, 1, &hSetWindowsHookExWHook);
    LhSetExclusiveACL(ACLEntries, 1, &hSetWindowsHookExAHook);

    cout << "[*] Keyboard deception active only inside injected process." << endl;
}