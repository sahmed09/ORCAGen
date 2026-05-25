#include <iostream>
#include <string>
#include <Windows.h>
#include <vector>
#include <algorithm>

// Function to check if a file matches our target patterns
bool IsTargetFile(const std::wstring& filename) {
    // Common extensions for sensitive files
    const std::vector<std::wstring> targetExtensions = {
        L".json", L".conf", L".ini", L".txt", L".xml", L".yml", L".yaml"
    };

    // Common keywords in filenames
    const std::vector<std::wstring> targetKeywords = {
        L"api_key", L"secret", L"password", L"credential",
        L"wallet", L"key", L"config", L"settings", L"login",
        L"auth", L"token", L"access", L"private", L"public"
    };

    // Convert filename to lowercase for case-insensitive comparison
    std::wstring lowerFilename = filename;
    std::transform(lowerFilename.begin(), lowerFilename.end(), lowerFilename.begin(), ::tolower);

    // Check extensions
    for (const auto& ext : targetExtensions) {
        if (lowerFilename.length() >= ext.length() &&
            lowerFilename.substr(lowerFilename.length() - ext.length()) == ext) {
            return true;
        }
    }

    // Check keywords
    for (const auto& keyword : targetKeywords) {
        if (lowerFilename.find(keyword) != std::wstring::npos) {
            return true;
        }
    }

    return false;
}

// Recursive function to scan directories
void ScanDirectory(const std::wstring& directoryPath, std::vector<std::wstring>& foundFiles) {
    WIN32_FIND_DATA findData;
    std::wstring searchPath = directoryPath + L"\\*";

    HANDLE hFind = FindFirstFile(searchPath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE) {
        return;
    }

    do {
        // Skip current and parent directory entries
        if (wcscmp(findData.cFileName, L".") != 0 && wcscmp(findData.cFileName, L"..") != 0) {
            std::wstring fullPath = directoryPath + L"\\" + findData.cFileName;

            // If it's a directory, recursively scan it
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                ScanDirectory(fullPath, foundFiles);
            }
            else {
                // Check if this file matches our target patterns
                if (IsTargetFile(findData.cFileName)) {
                    foundFiles.push_back(fullPath);
                }
            }
        }
    } while (FindNextFile(hFind, &findData));

    FindClose(hFind);
}

// Function to scan specific directory for sensitive files
std::vector<std::wstring> ScanForSensitiveFiles(const std::wstring& targetDirectory) {
    std::vector<std::wstring> foundFiles;
    ScanDirectory(targetDirectory, foundFiles);
    return foundFiles;
}

int main()
{
    std::string value;

    // Define the target directory to scan
    std::wstring targetDirectory = L"C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug";

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

        // Scan for sensitive files in target directory
        std::cout << "[*] Scanning for sensitive files...\n";
        std::vector<std::wstring> foundFiles = ScanForSensitiveFiles(targetDirectory);

        if (!foundFiles.empty()) {
            std::wcout << L"[!] Found " << foundFiles.size() << L" potentially sensitive files:\n";
            for (const auto& file : foundFiles) {
                std::wcout << L"  -> " << file << L"\n";
            }
        }
        else {
            std::cout << "[*] No sensitive files found in target directory.\n";
        }
    }

    return 0;
}


/*
#include <iostream>
#include <string>
#include <Windows.h>
#include <vector>
#include <algorithm>
#include <chrono>

// Function to check if a file matches our target patterns
bool IsTargetFile(const std::wstring& filename) {
    const std::vector<std::wstring> targetExtensions = {
        L".json", L".conf", L".ini", L".txt", L".xml", L".yml", L".yaml"
    };

    const std::vector<std::wstring> targetKeywords = {
        L"api_key", L"secret", L"password", L"credential",
        L"wallet", L"key", L"config", L"settings", L"login",
        L"auth", L"token", L"access", L"private", L"public"
    };

    std::wstring lowerFilename = filename;
    std::transform(lowerFilename.begin(), lowerFilename.end(), lowerFilename.begin(), ::tolower);

    for (const auto& ext : targetExtensions) {
        if (lowerFilename.length() >= ext.length() &&
            lowerFilename.substr(lowerFilename.length() - ext.length()) == ext) {
            return true;
        }
    }

    for (const auto& keyword : targetKeywords) {
        if (lowerFilename.find(keyword) != std::wstring::npos) {
            return true;
        }
    }

    return false;
}

// Recursive function to scan directories (with timing and count)
void ScanDirectory(const std::wstring& directoryPath,
    std::vector<std::wstring>& foundFiles,
    int& totalFiles,
    double& totalTimeMicroseconds)
{
    WIN32_FIND_DATA findData;
    std::wstring searchPath = directoryPath + L"\\*";

    HANDLE hFind = FindFirstFile(searchPath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE) {
        return;
    }

    do {
        if (wcscmp(findData.cFileName, L".") != 0 && wcscmp(findData.cFileName, L"..") != 0) {
            std::wstring fullPath = directoryPath + L"\\" + findData.cFileName;

            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                ScanDirectory(fullPath, foundFiles, totalFiles, totalTimeMicroseconds);
            }
            else {
                auto start = std::chrono::high_resolution_clock::now();

                bool isSensitive = IsTargetFile(findData.cFileName);

                auto end = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double, std::micro> duration = end - start;
                totalTimeMicroseconds += duration.count();
                ++totalFiles;

                if (isSensitive) {
                    foundFiles.push_back(fullPath);
                }
            }
        }
    } while (FindNextFile(hFind, &findData));

    FindClose(hFind);
}

// Function to scan specific directory for sensitive files
std::vector<std::wstring> ScanForSensitiveFiles(const std::wstring& targetDirectory,
    int& totalFiles,
    double& totalScanTimeMicroseconds)
{
    std::vector<std::wstring> foundFiles;
    totalFiles = 0;

    auto start = std::chrono::high_resolution_clock::now(); // Start timing

    ScanDirectory(targetDirectory, foundFiles, totalFiles, totalScanTimeMicroseconds);

    auto end = std::chrono::high_resolution_clock::now();   // End timing
    std::chrono::duration<double, std::micro> scanDuration = end - start;
    totalScanTimeMicroseconds = scanDuration.count();

    return foundFiles;
}

int main()
{
    std::string value;
    std::wstring targetDirectory = L"C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug";

    while (true)
    {
        HANDLE currentThread = GetCurrentThread();
        std::cout << "Process id: " << GetProcessIdOfThread(currentThread) << "\n";
        CloseHandle(currentThread);

        std::cout << "Press <enter> to Beep and Scan (Ctrl-C to exit): ";
        std::getline(std::cin, value);
        Beep(500, 500);

        std::cout << "[*] Scanning for sensitive files...\n";
        int totalFiles = 0;
        double totalScanTimeMicroseconds = 0.0;

        std::vector<std::wstring> foundFiles = ScanForSensitiveFiles(
            targetDirectory, totalFiles, totalScanTimeMicroseconds);

        if (!foundFiles.empty()) {
            std::wcout << L"[!] Found " << foundFiles.size() << L" potentially sensitive files:\n";
            for (const auto& file : foundFiles) {
                std::wcout << L"  -> " << file << L"\n";
            }
        }
        else {
            std::cout << "[*] No sensitive files found in target directory.\n";
        }

        std::cout << "[*] Scanned " << totalFiles << " files.\n";
        std::cout << "[*] Total Scan Time: " << totalScanTimeMicroseconds << " µs\n";

        if (totalFiles > 0) {
            double avgTime = totalScanTimeMicroseconds / totalFiles;
            std::cout << "[*] Average Time per File: " << avgTime << " µs\n";
        }
        else {
            std::cout << "[*] Average Time per File: N/A (no files scanned)\n";
        }

        std::cout << "==========================================" << std::endl << std::endl;
    }

    return 0;
}
*/