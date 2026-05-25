#include <iostream>
#include <string>
#include <fstream>
#include <Windows.h>
#include <sstream>
#include <ctime>

// Function to get current timestamp
std::string GetTimestamp()
{
    time_t now = time(0);
    char buf[80];
    struct tm timeinfo;
    localtime_s(&timeinfo, &now);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return std::string(buf);
}

// Function to log clipboard content to file
void LogClipboardContent(const std::string& content)
{
    std::ofstream logFile("clipboard_log.txt", std::ios::app);
    if (logFile.is_open())
    {
        logFile << "[" << GetTimestamp() << "] ";
        logFile << content << "\n";
        logFile << "----------------------------------------\n";
        logFile.close();
        std::cout << "[+] Clipboard content logged to clipboard_log.txt\n";
    }
    else
    {
        std::cerr << "[-] Failed to open log file\n";
    }
}

// Function to read clipboard data
bool ReadClipboard()
{
    if (!OpenClipboard(nullptr))
    {
        std::cerr << "[-] Failed to open clipboard\n";
        return false;
    }

    HANDLE hData = GetClipboardData(CF_TEXT);
    if (hData == nullptr)
    {
        std::cout << "[*] No text data in clipboard\n";
        CloseClipboard();
        return false;
    }

    char* pszText = static_cast<char*>(GlobalLock(hData));
    if (pszText == nullptr)
    {
        std::cerr << "[-] Failed to lock clipboard data\n";
        CloseClipboard();
        return false;
    }

    std::string clipboardText(pszText);
    GlobalUnlock(hData);
    CloseClipboard();

    if (!clipboardText.empty())
    {
        std::cout << "[+] Captured clipboard content:\n";
        std::cout << clipboardText << "\n";
        LogClipboardContent(clipboardText);
        return true;
    }

    return false;
}

int main()
{
    std::string value;

    std::cout << "====================================\n";
    std::cout << "Clipboard Logger PoC (for Research)\n";
    std::cout << "====================================\n\n";

    while (true)
    {
        // Output the current process Id
        HANDLE currentThread = GetCurrentThread();
        std::cout << "Process ID: ";
        std::cout << GetProcessIdOfThread(currentThread);
        std::cout << "\n";
        CloseHandle(currentThread);

        std::cout << "\nPress <enter> to:\n";
        std::cout << "  1. Beep\n";
        std::cout << "  2. Read & Log Clipboard\n";
        std::cout << "(Ctrl-C to exit): ";

        std::getline(std::cin, value);

        // Trigger beep
        Beep(500, 500);

        // Attempt to read clipboard
        ReadClipboard();

        std::cout << "\n";
    }

    return 0;
}


/*
#include <iostream>
#include <string>
#include <fstream>
#include <Windows.h>
#include <sstream>
#include <ctime>
#include <chrono>   // <-- Added for overhead calculation

// Function to get current timestamp
std::string GetTimestamp()
{
    time_t now = time(0);
    char buf[80];
    struct tm timeinfo;
    localtime_s(&timeinfo, &now);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return std::string(buf);
}

// Function to log clipboard content to file
void LogClipboardContent(const std::string& content)
{
    std::ofstream logFile("clipboard_log.txt", std::ios::app);
    if (logFile.is_open())
    {
        logFile << "[" << GetTimestamp() << "] ";
        logFile << content << "\n";
        logFile << "----------------------------------------\n";
        logFile.close();
        std::cout << "[+] Clipboard content logged to clipboard_log.txt\n";
    }
    else
    {
        std::cerr << "[-] Failed to open log file\n";
    }
}

// Function to read clipboard data
bool ReadClipboard()
{
    if (!OpenClipboard(nullptr))
    {
        std::cerr << "[-] Failed to open clipboard\n";
        return false;
    }

    HANDLE hData = GetClipboardData(CF_TEXT);
    if (hData == nullptr)
    {
        std::cout << "[*] No text data in clipboard\n";
        CloseClipboard();
        return false;
    }

    char* pszText = static_cast<char*>(GlobalLock(hData));
    if (pszText == nullptr)
    {
        std::cerr << "[-] Failed to lock clipboard data\n";
        CloseClipboard();
        return false;
    }

    std::string clipboardText(pszText);
    GlobalUnlock(hData);
    CloseClipboard();

    if (!clipboardText.empty())
    {
        std::cout << "[+] Captured clipboard content:\n";
        std::cout << clipboardText << "\n";
        LogClipboardContent(clipboardText);
        return true;
    }

    return false;
}

int main()
{
    std::string value;

    std::cout << "====================================\n";
    std::cout << "Clipboard Logger PoC (for Research)\n";
    std::cout << "====================================\n\n";

    while (true)
    {
        // Output the current process Id
        HANDLE currentThread = GetCurrentThread();
        std::cout << "Process ID: ";
        std::cout << GetProcessIdOfThread(currentThread);
        std::cout << "\n";
        CloseHandle(currentThread);

        std::cout << "\nPress <enter> to:\n";
        std::cout << "  1. Beep\n";
        std::cout << "  2. Read & Log Clipboard\n";
        std::cout << "(Ctrl-C to exit): ";

        std::getline(std::cin, value);

        // Trigger beep
        Beep(500, 500);

        // --------------------------------------------
        // Overhead timing begins
        // --------------------------------------------
        auto start = std::chrono::high_resolution_clock::now();

        // Attempt to read clipboard
        ReadClipboard();

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::micro> duration = end - start;

        std::cout << "[Time] Clipboard read took "
                  << duration.count() << " microseconds.\n";
        // --------------------------------------------

        std::cout << "\n";
    }

    return 0;
}
*/