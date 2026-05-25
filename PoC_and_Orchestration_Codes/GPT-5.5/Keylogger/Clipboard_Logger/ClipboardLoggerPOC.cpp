#include <iostream>
#include <string>
#include <fstream>
#include <Windows.h>
#include <chrono>

void LogClipboardText()
{
    if (!OpenClipboard(nullptr))
    {
        std::cout << "[-] Failed to open clipboard.\n";
        return;
    }

    HANDLE hData = GetClipboardData(CF_TEXT);
    if (hData == nullptr)
    {
        std::cout << "[-] No CF_TEXT data available in clipboard.\n";
        CloseClipboard();
        return;
    }

    char* pszText = static_cast<char*>(GlobalLock(hData));
    if (pszText == nullptr)
    {
        std::cout << "[-] Failed to lock clipboard data.\n";
        CloseClipboard();
        return;
    }

    std::ofstream logFile("clipboard_log.txt", std::ios::app);
    if (logFile.is_open())
    {
        logFile << "[Clipboard Capture] " << pszText << std::endl;
        logFile.close();

        std::cout << "[+] Clipboard content saved to clipboard_log.txt\n";
    }
    else
    {
        std::cout << "[-] Failed to open clipboard_log.txt\n";
    }

    GlobalUnlock(hData);
    CloseClipboard();
}

int main()
{
    std::string value;

    while (true)
    {
        HANDLE currentThread = GetCurrentThread();

        std::cout << "Process id: ";
        std::cout << GetProcessIdOfThread(currentThread);
        std::cout << "\n";

        CloseHandle(currentThread);

        std::cout << "Press <enter> to Beep and log clipboard content (Ctrl-C to exit): ";
        std::getline(std::cin, value);

        auto start = std::chrono::high_resolution_clock::now();

        LogClipboardText();

        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double, std::micro> duration = end - start;
        std::cout << "[Time] Clipboard read took " << duration.count() << " microseconds.\n";

        Beep(500, 500);
    }

    return 0;
}

/*
#include <iostream>
#include <string>
#include <fstream>
#include <Windows.h>

void LogClipboardText()
{
    if (!OpenClipboard(nullptr))
    {
        std::cout << "[-] Failed to open clipboard.\n";
        return;
    }

    HANDLE hData = GetClipboardData(CF_TEXT);
    if (hData == nullptr)
    {
        std::cout << "[-] No CF_TEXT data available in clipboard.\n";
        CloseClipboard();
        return;
    }

    char* pszText = static_cast<char*>(GlobalLock(hData));
    if (pszText == nullptr)
    {
        std::cout << "[-] Failed to lock clipboard data.\n";
        CloseClipboard();
        return;
    }

    std::ofstream logFile("clipboard_log.txt", std::ios::app);
    if (logFile.is_open())
    {
        logFile << "[Clipboard Capture] " << pszText << std::endl;
        logFile.close();

        std::cout << "[+] Clipboard content saved to clipboard_log.txt\n";
    }
    else
    {
        std::cout << "[-] Failed to open clipboard_log.txt\n";
    }

    GlobalUnlock(hData);
    CloseClipboard();
}

int main()
{
    std::string value;

    while (true)
    {
        HANDLE currentThread = GetCurrentThread();

        std::cout << "Process id: ";
        std::cout << GetProcessIdOfThread(currentThread);
        std::cout << "\n";

        CloseHandle(currentThread);

        std::cout << "Press <enter> to Beep and log clipboard content (Ctrl-C to exit): ";
        std::getline(std::cin, value);

        LogClipboardText();

        Beep(500, 500);
    }

    return 0;
}
*/