#include <iostream>
#include <string>
#include <vector>
#include <Windows.h>
#include <chrono>

int totalFiles = 0;

// Helper function to check if a filename matches high-value targets
bool IsSensitiveFile(const std::wstring& filename)
{
    // High-value extensions and keywords to flag
    std::vector<std::wstring> targets = {
        L".json", L".conf", L".ini", L".txt",
        L"api_key", L"wallet.dat", L"profile"
    };

    // Convert filename to lowercase for case-insensitive matching
    std::wstring lowerFilename = filename;
    for (auto& c : lowerFilename) c = towlower(c);

    for (const auto& target : targets)
    {
        if (lowerFilename.find(target) != std::wstring::npos)
        {
            return true;
        }
    }
    return false;
}

// Recursively scans the target directory using Windows APIs
// Recursively scans the target directory using Windows APIs
void ScanDirectory(const std::wstring& searchPath)
{
    std::wstring searchPattern = searchPath + L"\\*";
    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(searchPattern.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        std::wcerr << L"[-] Could not open directory: " << searchPath << std::endl;
        return;
    }

    do
    {
        // FIX: Changed findData.c_str to findData.cFileName
        std::wstring currentName = findData.cFileName;

        // Skip the relative directory links "." and ".."
        if (currentName == L"." || currentName == L"..")
        {
            continue;
        }

        std::wstring fullPath = searchPath + L"\\" + currentName;

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            // Recursively scan subdirectories
            ScanDirectory(fullPath);
        }
        else
        {
            ++totalFiles;

            // Process file and evaluate if it matches our sensitive patterns
            if (IsSensitiveFile(currentName))
            {
                std::wcout << L"[FOUND SENSITIVE] " << fullPath << std::endl;
            }
            else
            {
                std::wcout << L"[FOUND GENERIC] " << fullPath << std::endl;
            }
        }

    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);
}

int main()
{
    std::string value;

    // Target directory specified for the simulation environment
    std::wstring targetDirectory = L"C:\\Users\\User\\Documents\\MalwareAnalysis\\ClipboardLogger\\Debug";

    while (true)
    {
        // Output the current process Id
        HANDLE currentThread = GetCurrentThread();
        std::cout << "\n========================================\n";
        std::cout << "Process id: " << GetProcessIdOfThread(currentThread) << "\n";
        CloseHandle(currentThread);

        std::cout << "Press <enter> to Beep (Ctrl-C to exit): ";
        std::getline(std::cin, value);
        Beep(500, 500);

        std::cout << "Starting simulated file system scan...\n";

        totalFiles = 0;
        auto start = std::chrono::high_resolution_clock::now();

        ScanDirectory(targetDirectory);

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::micro> duration = end - start;

        std::cout << "Scan complete.\n";
        std::cout << "[*] Scanned " << totalFiles << " files.\n";
        std::cout << "[*] Total Scan Time: " << duration.count() << " µs\n";

        if (totalFiles > 0)
        {
            double avgTime = duration.count() / totalFiles;
            std::cout << "[*] Average Time per File: " << avgTime << " µs\n";
        }
        else
        {
            std::cout << "[*] Average Time per File: N/A (no files scanned)\n";
        }
    }

    return 0;
}

/*
#include <iostream>
#include <string>
#include <vector>
#include <Windows.h>

// Helper function to check if a filename matches high-value targets
bool IsSensitiveFile(const std::wstring& filename)
{
    // High-value extensions and keywords to flag
    std::vector<std::wstring> targets = {
        L".json", L".conf", L".ini", L".txt",
        L"api_key", L"wallet.dat", L"profile"
    };

    // Convert filename to lowercase for case-insensitive matching
    std::wstring lowerFilename = filename;
    for (auto& c : lowerFilename) c = towlower(c);

    for (const auto& target : targets)
    {
        if (lowerFilename.find(target) != std::wstring::npos)
        {
            return true;
        }
    }
    return false;
}

// Recursively scans the target directory using Windows APIs
// Recursively scans the target directory using Windows APIs
void ScanDirectory(const std::wstring& searchPath)
{
    std::wstring searchPattern = searchPath + L"\\*";
    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(searchPattern.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        std::wcerr << L"[-] Could not open directory: " << searchPath << std::endl;
        return;
    }

    do
    {
        // FIX: Changed findData.c_str to findData.cFileName
        std::wstring currentName = findData.cFileName;

        // Skip the relative directory links "." and ".."
        if (currentName == L"." || currentName == L"..")
        {
            continue;
        }

        std::wstring fullPath = searchPath + L"\\" + currentName;

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            // Recursively scan subdirectories
            ScanDirectory(fullPath);
        }
        else
        {
            // Process file and evaluate if it matches our sensitive patterns
            if (IsSensitiveFile(currentName))
            {
                std::wcout << L"[FOUND SENSITIVE] " << fullPath << std::endl;
            }
            else
            {
                std::wcout << L"[FOUND GENERIC] " << fullPath << std::endl;
            }
        }

    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);
}

int main()
{
    std::string value;

    // Target directory specified for the simulation environment
    std::wstring targetDirectory = L"C:\\Users\\User\\Documents\\MalwareAnalysis\\ClipboardLogger\\Debug";

    while (true)
    {
        // Output the current process Id
        HANDLE currentThread = GetCurrentThread();
        std::cout << "\n========================================\n";
        std::cout << "Process id: " << GetProcessIdOfThread(currentThread) << "\n";
        CloseHandle(currentThread);

        std::cout << "Press <enter> to Beep (Ctrl-C to exit): ";
        std::getline(std::cin, value);
        Beep(500, 500);

        std::cout << "Starting simulated file system scan...\n";
        ScanDirectory(targetDirectory);
        std::cout << "Scan complete.\n";
    }
    return 0;
}
*/