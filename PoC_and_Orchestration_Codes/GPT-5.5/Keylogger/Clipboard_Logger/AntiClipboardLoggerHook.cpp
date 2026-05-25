#include "pch.h"
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <iostream>
#include <string>
#include <easyhook.h>

#pragma comment(lib, "EasyHook32.lib") // Use EasyHook64.lib for x64 builds

using namespace std;

static HGLOBAL gDecoyClipboardHandle = nullptr;

HANDLE WINAPI myGetClipboardDataHook(UINT uFormat)
{
    cout << "[Hook] GetClipboardData intercepted. Format: " << uFormat << endl;

    if (uFormat == CF_TEXT)
    {
        const char* decoyText =
            "DECOY_CLIPBOARD_DATA: fake_password_1234";

        SIZE_T dataSize = strlen(decoyText) + 1;

        if (gDecoyClipboardHandle != nullptr)
        {
            GlobalFree(gDecoyClipboardHandle);
            gDecoyClipboardHandle = nullptr;
        }

        gDecoyClipboardHandle = GlobalAlloc(GMEM_MOVEABLE, dataSize);
        if (gDecoyClipboardHandle == nullptr)
        {
            cout << "[-] Failed to allocate decoy clipboard memory." << endl;
            return GetClipboardData(uFormat);
        }

        void* lockedMemory = GlobalLock(gDecoyClipboardHandle);
        if (lockedMemory == nullptr)
        {
            cout << "[-] Failed to lock decoy clipboard memory." << endl;
            GlobalFree(gDecoyClipboardHandle);
            gDecoyClipboardHandle = nullptr;
            return GetClipboardData(uFormat);
        }

        memcpy(lockedMemory, decoyText, dataSize);
        GlobalUnlock(gDecoyClipboardHandle);

        cout << "[Deception] Returned decoy clipboard content." << endl;

        return gDecoyClipboardHandle;
    }

    return GetClipboardData(uFormat);
}

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(
    REMOTE_ENTRY_INFO * inRemoteInfo)
{
    cout << "[*] Clipboard deception DLL injected." << endl;

    HOOK_TRACE_INFO hGetClipboardDataHook = { NULL };

    FARPROC getClipboardDataAddr =
        GetProcAddress(GetModuleHandle(TEXT("user32")), "GetClipboardData");

    if (getClipboardDataAddr == nullptr)
    {
        cout << "[-] Failed to locate GetClipboardData." << endl;
        return;
    }

    cout << "[*] GetClipboardData address: "
        << reinterpret_cast<void*>(getClipboardDataAddr)
        << endl;

    NTSTATUS result = LhInstallHook(
        getClipboardDataAddr,
        myGetClipboardDataHook,
        nullptr,
        &hGetClipboardDataHook
    );

    if (FAILED(result))
    {
        wcout << L"[-] Failed to install GetClipboardData hook: "
            << RtlGetLastErrorString()
            << endl;
        return;
    }

    cout << "[+] GetClipboardData hook installed successfully." << endl;

    ULONG ACLEntries[1] = { 0 };
    LhSetExclusiveACL(ACLEntries, 1, &hGetClipboardDataHook);

    cout << "[+] Hook ACL configured." << endl;
}