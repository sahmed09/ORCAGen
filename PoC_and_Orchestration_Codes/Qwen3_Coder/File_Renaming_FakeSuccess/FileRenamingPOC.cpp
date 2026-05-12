#include <iostream>
#include <string>
#include <Windows.h>
#include <vector>
#include <algorithm>
#include <iomanip>

// Function to generate a simple base64-like encoded string for demonstration
std::wstring generateDummyKey() {
    // This is just a placeholder - in real malware this would be the actual encryption key
    return L"V2luZG93c0FwaVNlY3VyaXR5";
}

// Function to rename files with ransomware-style extensions
bool renameEncryptedFiles(const std::wstring& directory) {
    std::wstring searchPath = directory + L"\\*.*";
    WIN32_FIND_DATA findData;
    HANDLE hFind = FindFirstFile(searchPath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE) {
        std::wcout << L"Failed to open directory: " << directory << std::endl;
        return false;
    }

    // List of file extensions that are typically encrypted
    std::vector<std::wstring> encryptableExtensions = { L".txt", L".doc", L".docx", L".xls", L".xlsx",
                                                      L".ppt", L".pptx", L".pdf", L".jpg", L".png",
                                                      L".gif", L".bmp", L".mp3", L".mp4", L".avi",
                                                      L".mov", L".wmv", L".flv", L".mkv", L".zip" };

    // Get base64 encoded key for the extension
    std::wstring key = generateDummyKey();

    do {
        // Skip directories and system files
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            continue;
        }

        // Get full file path
        std::wstring fileName = findData.cFileName;
        std::wstring fullPath = directory + L"\\" + fileName;

        // Check if file has encryptable extension
        size_t dotPos = fileName.find_last_of(L'.');
        if (dotPos != std::wstring::npos) {
            std::wstring extension = fileName.substr(dotPos);
            // Convert extension to lowercase for comparison
            std::transform(extension.begin(), extension.end(), extension.begin(), ::towlower);

            // Check if extension is in our list of encryptable files
            bool isEncryptable = false;
            for (const auto& ext : encryptableExtensions) {
                if (extension == ext) {
                    isEncryptable = true;
                    break;
                }
            }

            if (isEncryptable) {
                // Create new filename with ransomware extension
                std::wstring newFileName = fileName.substr(0, dotPos) + L".locked";

                // Add key identifier to extension (simulated)
                if (!key.empty()) {
                    newFileName = fileName.substr(0, dotPos) + L"." + key.substr(0, 6) + L".locked";
                }
                else {
                    newFileName = fileName.substr(0, dotPos) + L".locked";
                }

                std::wstring newFullPath = directory + L"\\" + newFileName;

                // Rename file
                if (MoveFileEx(fullPath.c_str(), newFullPath.c_str(), MOVEFILE_REPLACE_EXISTING)) {
                    std::wcout << L"[+] Renamed: " << fileName << L" -> " << newFileName << std::endl;
                }
                else {
                    std::wcout << L"[-] Failed to rename: " << fileName << std::endl;
                }
            }
        }
    } while (FindNextFile(hFind, &findData));

    FindClose(hFind);
    return true;
}

int main()
{
    std::string value;
    std::wstring targetDirectory = L"C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug\\Files";

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

        // Perform beep operation
        Beep(500, 500);

        // Add file renaming functionality every few iterations
        std::wcout << L"[*] Performing file encryption simulation..." << std::endl;
        renameEncryptedFiles(targetDirectory);
    }

    return 0;
}


/*
#include <iostream>
#include <string>
#include <Windows.h>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <chrono>

// Function to generate a simple base64-like encoded string for demonstration
std::wstring generateDummyKey() {
    // This is just a placeholder - in real malware this would be the actual encryption key
    return L"V2luZG93c0FwaVNlY3VyaXR5";
}

// Function to rename files with ransomware-style extensions and measure timing
bool renameEncryptedFiles(const std::wstring& directory) {
    std::wstring searchPath = directory + L"\\*.*";
    WIN32_FIND_DATA findData;
    HANDLE hFind = FindFirstFile(searchPath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE) {
        std::wcout << L"Failed to open directory: " << directory << std::endl;
        return false;
    }

    std::vector<std::wstring> encryptableExtensions = {
        L".txt", L".doc", L".docx", L".xls", L".xlsx", L".ppt", L".pptx", L".pdf",
        L".jpg", L".png", L".gif", L".bmp", L".mp3", L".mp4", L".avi", L".mov",
        L".wmv", L".flv", L".mkv", L".zip"
    };

    std::wstring key = generateDummyKey();
    int attemptCount = 0;
    double totalMicroseconds = 0.0;

    do {
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;

        std::wstring fileName = findData.cFileName;
        std::wstring fullPath = directory + L"\\" + fileName;

        size_t dotPos = fileName.find_last_of(L'.');
        if (dotPos != std::wstring::npos) {
            std::wstring extension = fileName.substr(dotPos);
            std::transform(extension.begin(), extension.end(), extension.begin(), ::towlower);

            bool isEncryptable = std::find(encryptableExtensions.begin(), encryptableExtensions.end(), extension) != encryptableExtensions.end();

            if (isEncryptable) {
                std::wstring baseName = fileName.substr(0, dotPos);
                std::wstring newFileName;

                if (!key.empty()) {
                    newFileName = baseName + L"." + key.substr(0, 6) + L".locked";
                }
                else {
                    newFileName = baseName + L".locked";
                }

                std::wstring newFullPath = directory + L"\\" + newFileName;

                auto start = std::chrono::high_resolution_clock::now();
                BOOL result = MoveFileEx(fullPath.c_str(), newFullPath.c_str(), MOVEFILE_REPLACE_EXISTING);
                auto end = std::chrono::high_resolution_clock::now();

                std::chrono::duration<double, std::micro> duration = end - start;
                totalMicroseconds += duration.count();
                ++attemptCount;

                if (result) {
                    std::wcout << L"[+] Renamed: " << fileName << L" -> " << newFileName
                        << L" (Time: " << duration.count() << L" µs)\n";
                }
                else {
                    std::wcerr << L"[-] Failed to rename: " << fileName
                        << L" (FakeSuccess or real failure) - Time: " << duration.count() << L" µs\n";
                }
            }
        }
    } while (FindNextFile(hFind, &findData));

    FindClose(hFind);

    if (attemptCount > 0) {
        double avgTime = totalMicroseconds / attemptCount;
        std::wcout << L"\n[*] Total files attempted: " << attemptCount << L"\n";
        std::wcout << L"[*] Total time spent: " << totalMicroseconds << L" µs\n";
        std::wcout << L"[*] Average time per attempt: " << avgTime << L" µs\n";
    }
    else {
        std::wcout << L"[!] No target files found for encryption.\n";
    }

    return true;
}

int main() {
    std::string value;
    std::wstring targetDirectory = L"C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug\\Files";

    while (true) {
        HANDLE currentThread = GetCurrentThread();
        std::cout << "Process id: " << GetProcessIdOfThread(currentThread) << "\n";
        CloseHandle(currentThread);

        std::cout << "Press <enter> to Beep and simulate ransomware rename (Ctrl-C to exit): ";
        std::getline(std::cin, value);

        Beep(500, 500);
        std::wcout << L"\n[*] Performing file encryption simulation...\n";
        renameEncryptedFiles(targetDirectory);
        std::wcout << L"[*] Simulation complete.\n\n";
    }

    return 0;
}
*/
