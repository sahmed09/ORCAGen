#include <iostream>
#include <string>
#include <Windows.h>
#include <fstream>
#include <ctime>
#include <chrono>

// Function to log clipboard data to a file
void LogClipboardData()
{
    // Open the clipboard
    if (OpenClipboard(NULL))
    {
        // Get handle to clipboard data (CF_TEXT format)
        HGLOBAL hData = GetClipboardData(CF_TEXT);
        if (hData != NULL)
        {
            // Lock the global memory and get a pointer to the data
            char* pszText = (char*)GlobalLock(hData);
            if (pszText != NULL)
            {
                // Create timestamp for log file
                time_t now = time(0);
                char buffer[26];
                ctime_s(buffer, sizeof(buffer), &now);
                buffer[strcspn(buffer, "\n")] = '\0'; // Remove newline

                // Write to log file
                std::ofstream logFile("clipboard_log.txt", std::ios::app);
                if (logFile.is_open())
                {
                    logFile << "[" << buffer << "] Clipboard Data: " << pszText << std::endl;
                    logFile.close();
                    std::cout << "[*] Clipboard data logged to clipboard_log.txt" << std::endl;
                }

                // Unlock the global memory
                GlobalUnlock(hData);
            }
        }
        // Close the clipboard
        CloseClipboard();
    }
}

int main()
{
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

        // Log clipboard data before beeping
        auto start = std::chrono::high_resolution_clock::now();
        LogClipboardData();
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double, std::micro> duration = end - start;
        std::cout << "[Time] Clipboard read took " << duration.count() << " microseconds.\n";

        Beep(500, 500);
    }
    return 0;
}