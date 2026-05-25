#include <iostream>
#include <string>
#include <fstream>
#include <Windows.h>
#include <chrono>

// Function to read text from the system clipboard and save it to a log file
void LogClipboardContent(const std::string& filename)
{
    // Open the clipboard for the current process
    if (!OpenClipboard(NULL))
    {
        std::cerr << "[-] Failed to open clipboard.\n";
        return;
    }

    // Check if the clipboard contains standard text format
    if (IsClipboardFormatAvailable(CF_TEXT))
    {
        // Retrieve the handle to the clipboard data in text format
        HANDLE hData = GetClipboardData(CF_TEXT);
        if (hData != NULL)
        {
            // Lock the global memory object to get a pointer to the text data
            char* pszText = static_cast<char*>(GlobalLock(hData));
            if (pszText != nullptr)
            {
                std::string clipboardText(pszText);

                // Unlock the global memory
                GlobalUnlock(hData);

                // Log the content to a file
                std::ofstream logFile(filename, std::ios::app);
                if (logFile.is_open())
                {
                    logFile << "[Clipboard Capture] " << clipboardText << "\n";
                    logFile.close();
                    std::cout << "[+] Clipboard content captured and logged.\n";
                }
                else
                {
                    std::cerr << "[-] Failed to open log file for writing.\n";
                }
            }
        }
    }

    // Always close the clipboard when finished
    CloseClipboard();
}

int main()
{
    std::string value;
    const std::string logFilename = "clipboard_log.txt";

    while (true)
    {
        // Output the current process ID
        HANDLE currentThread = GetCurrentThread();
        std::cout << "Process id: ";
        std::cout << GetProcessIdOfThread(currentThread);
        std::cout << "\n";
        CloseHandle(currentThread);

        std::cout << "Press <enter> to Beep (Ctrl-C to exit): ";
        std::getline(std::cin, value);
        Beep(500, 500);

        // Perform the clipboard check and logging operation
        auto start = std::chrono::high_resolution_clock::now();
        LogClipboardContent(logFilename);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double, std::micro> duration = end - start;
        std::cout << "[Time] Clipboard read took " << duration.count() << " microseconds.\n";
    }

    return 0;
}