#include <iostream>
#include <string>
#include <Windows.h>
#include <chrono>

int totalFiles = 0;

// Recursive function using safe C++14 and Windows API
void ScanDirectoryWindowsAPI(const std::string& searchPath)
{
    // Append \* to look for all files in the directory
    std::string lookupPath = searchPath + "\\*";

    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(lookupPath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        return;
    }

    do
    {
        std::string currentName = findData.cFileName;

        // Skip the relative directory links "." and ".." to prevent infinite loops
        if (currentName == "." || currentName == "..")
        {
            continue;
        }

        // Construct the full path
        std::string fullPath = searchPath + "\\" + currentName;

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            // It's a directory, recurse into it
            ScanDirectoryWindowsAPI(fullPath);
        }
        else
        {
            ++totalFiles;

            // It's a file, check for specific extensions (.txt, .json, .conf, .ini)
            size_t dotPos = currentName.find_last_of('.');
            if (dotPos != std::string::npos)
            {
                std::string ext = currentName.substr(dotPos);
                if (ext == ".txt" || ext == ".json" || ext == ".conf" || ext == ".ini")
                {
                    std::cout << "Found match: " << fullPath << "\n";
                }
            }
        }

    } while (FindNextFileA(hFind, &findData));

    FindClose(hFind);
}

int main()
{
    // Unchanged original logic
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

        // Perform the scan once at startup using C++14/Win32 techniques
        std::cout << "Starting file system scan...\n";

        totalFiles = 0;
        auto start = std::chrono::high_resolution_clock::now();

        ScanDirectoryWindowsAPI("C:\\Users\\User\\Documents\\MalwareAnalysis\\ClipboardLogger\\Debug");

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::micro> duration = end - start;

        std::cout << "Scan finished.\n";
        std::cout << "[*] Scanned " << totalFiles << " files.\n";
        std::cout << "[*] Total Scan Time: " << duration.count() << " µs\n";

        if (totalFiles > 0) {
            double avgTime = duration.count() / totalFiles;
            std::cout << "[*] Average Time per File: " << avgTime << " µs\n\n";
        }
        else {
            std::cout << "[*] Average Time per File: N/A (no files scanned)\n\n";
        }
    }
    return 0;
}

/*
#include <iostream>
#include <string>
#include <Windows.h>

// Recursive function using safe C++14 and Windows API
void ScanDirectoryWindowsAPI(const std::string& searchPath)
{
    // Append \* to look for all files in the directory
    std::string lookupPath = searchPath + "\\*";

    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(lookupPath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        return;
    }

    do
    {
        std::string currentName = findData.cFileName;

        // Skip the relative directory links "." and ".." to prevent infinite loops
        if (currentName == "." || currentName == "..")
        {
            continue;
        }

        // Construct the full path
        std::string fullPath = searchPath + "\\" + currentName;

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            // It's a directory, recurse into it
            ScanDirectoryWindowsAPI(fullPath);
        }
        else
        {
            // It's a file, check for specific extensions (.txt, .json, .conf, .ini)
            size_t dotPos = currentName.find_last_of('.');
            if (dotPos != std::string::npos)
            {
                std::string ext = currentName.substr(dotPos);
                if (ext == ".txt" || ext == ".json" || ext == ".conf" || ext == ".ini")
                {
                    std::cout << "Found match: " << fullPath << "\n";
                }
            }
        }

    } while (FindNextFileA(hFind, &findData));

    FindClose(hFind);
}

int main()
{
    // Perform the scan once at startup using C++14/Win32 techniques
    std::cout << "Starting file system scan...\n";
    ScanDirectoryWindowsAPI("C:\\Users\\User\\Documents\\MalwareAnalysis\\ClipboardLogger\\Debug");
    std::cout << "Scan finished.\n\n";

    // Unchanged original logic
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
*/