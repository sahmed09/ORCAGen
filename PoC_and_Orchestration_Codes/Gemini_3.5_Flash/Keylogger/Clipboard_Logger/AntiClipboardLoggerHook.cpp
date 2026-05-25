#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <easyhook.h>

#ifdef _WIN64
#pragma comment(lib, "EasyHook64.lib")
#else
#pragma comment(lib, "EasyHook32.lib")
#endif

using namespace std;

// Global handle used to track dynamic memory allocated for the decoy data
HGLOBAL g_hDecoyMemory = NULL;

// Function pointer signature for the original GetClipboardData API
typedef HANDLE(WINAPI* PFN_GetClipboardData)(UINT uFormat);

// Decoy content meant to mislead the cyber threat actor / malware data harvester
const char* DECOY_CREDENTIAL = "[HONEYTOKEN] user=\"admin_prod\" pass=\"SuperSecretDeceptivePass2026!\"";

// Hook function for GetClipboardData
HANDLE WINAPI myGetClipboardDataHook(UINT uFormat)
{
    // Check if the targeted application is trying to harvest standard text data
    if (uFormat == CF_TEXT || uFormat == CF_OEMTEXT)
    {
        cout << "\n[+] Deception Triggered: Intercepted GetClipboardData for text format." << endl;

        // Free previous decoy allocation if it exists to prevent potential memory leaks across loops
        if (g_hDecoyMemory != NULL)
        {
            GlobalFree(g_hDecoyMemory);
            g_hDecoyMemory = NULL;
        }

        size_t decoyLen = strlen(DECOY_CREDENTIAL) + 1;

        // Allocate global memory required by the Windows Clipboard architecture (GHND = GMEM_MOVEABLE | GMEM_ZEROINIT)
        g_hDecoyMemory = GlobalAlloc(GHND, decoyLen);
        if (g_hDecoyMemory != NULL)
        {
            // Lock the memory block to copy our decoy text into it safely
            char* pDest = static_cast<char*>(GlobalLock(g_hDecoyMemory));
            if (pDest != nullptr)
            {
                memcpy(pDest, DECOY_CREDENTIAL, decoyLen);
                GlobalUnlock(g_hDecoyMemory);

                cout << "[Deception] Substituted genuine clipboard content with honeytoken payload." << endl;

                // Return the handle to our decoy memory block instead of the real data
                return g_hDecoyMemory;
            }
        }
    }

    // Fallback or execution pass-through for unhandled formats (e.g., CF_BITMAP)
    // Dynamic lookups avoid compilation dependency tangles inside the wrapper execution block
    PFN_GetClipboardData pfnGetClipboardData = (PFN_GetClipboardData)GetProcAddress(GetModuleHandle(TEXT("user32")), "GetClipboardData");
    return pfnGetClipboardData(uFormat);
}

// EasyHook Dynamic Link Library Injection Entry Point
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
    cout << "[*] Injection started inside target process." << endl;

    HOOK_TRACE_INFO hGetClipboardDataHook = { NULL };

    // Locate the exact address of GetClipboardData inside user32.dll
    FARPROC getClipboardDataAddr = GetProcAddress(GetModuleHandle(TEXT("user32")), "GetClipboardData");

    if (getClipboardDataAddr == nullptr)
    {
        cerr << "[-] Failed to locate GetClipboardData export address." << endl;
        return;
    }

    // Install the runtime hook redirection layout
    NTSTATUS result = LhInstallHook(
        getClipboardDataAddr,
        myGetClipboardDataHook,
        nullptr,
        &hGetClipboardDataHook
    );

    if (FAILED(result))
    {
        wstring errStr(RtlGetLastErrorString());
        wcerr << L"[-] Failed to install GetClipboardData hook: " << errStr << endl;
        return;
    }
    else
    {
        cout << "[+] GetClipboardData hook installed successfully!" << endl;
    }

    // Configure the Access Control List (ACL). 
    // Setting an empty exclusive ACL activates the hook explicitly for all threads within this target process only.
    ULONG ACLEntries[1] = { 0 };
    LhSetExclusiveACL(ACLEntries, 1, &hGetClipboardDataHook);
}