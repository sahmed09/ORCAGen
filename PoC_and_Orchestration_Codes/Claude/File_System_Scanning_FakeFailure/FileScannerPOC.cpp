#include <iostream>
#include <string>
#include <vector>
#include <Windows.h>
#include <algorithm>
#include <chrono>
#include <iomanip>

// Target extensions and keywords
const std::vector<std::string> targetExtensions = { ".json", ".conf", ".ini", ".txt" };
const std::vector<std::string> targetKeywords = { "api_key", "wallet.dat", "config", "credential", "password", "secret", "profile" };

// Utility: case-insensitive search
bool containsInsensitive(const std::string& haystack, const std::string& needle) {
    auto it = std::search(
        haystack.begin(), haystack.end(),
        needle.begin(), needle.end(),
        [](char ch1, char ch2) { return std::tolower(ch1) == std::tolower(ch2); }
    );
    return (it != haystack.end());
}

// Check if filename matches sensitive patterns
bool isSensitiveFile(const std::string& filename) {
    for (const auto& ext : targetExtensions) {
        if (filename.length() >= ext.length() &&
            std::equal(ext.rbegin(), ext.rend(), filename.rbegin(),
                [](char a, char b) { return std::tolower(a) == std::tolower(b); }))
            return true;
    }
    for (const auto& keyword : targetKeywords) {
        if (containsInsensitive(filename, keyword))
            return true;
    }
    return false;
}

// Recursive file scanner
void scanDirectory(const std::string& directory) {
    std::string searchPath = directory + "\\*";
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE)
        return;

    do {
        const std::string name = findData.cFileName;
        if (name == "." || name == "..")
            continue;

        std::string fullPath = directory + "\\" + name;

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            scanDirectory(fullPath);  // Recurse into subdirectories
        }
        else {
            if (isSensitiveFile(name)) {
                std::cout << "[Sensitive File Found]: " << fullPath << std::endl;
            }
        }
    } while (FindNextFileA(hFind, &findData));
    FindClose(hFind);
}

int main()
{
    std::string value;
    const std::string rootDir = "C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug";
    std::vector<double> scanTimesMicroseconds;

    while (true)
    {
        // Output the current process Id
        HANDLE currentThread = GetCurrentThread();
        std::cout << "Process id: " << GetProcessIdOfThread(currentThread) << "\n";
        CloseHandle(currentThread);

        std::cout << "Press <enter> to Beep and Scan 10x (Ctrl-C to exit): ";
        std::getline(std::cin, value);
        Beep(500, 500);

        scanTimesMicroseconds.clear();

        std::cout << "[*] Performing 10 scans in: " << rootDir << std::endl;

        for (int i = 0; i < 10; ++i)
        {
            auto start = std::chrono::high_resolution_clock::now();

            scanDirectory(rootDir);

            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::micro> duration = end - start;
            double timeTaken = duration.count();

            std::cout << "[*] Scan " << (i + 1) << " Time: " << std::fixed << std::setprecision(2)
                << timeTaken << " µs" << std::endl;

            scanTimesMicroseconds.push_back(timeTaken);
        }

        // Compute average
        double total = 0.0;
        for (const auto& t : scanTimesMicroseconds)
            total += t;

        double average = total / scanTimesMicroseconds.size();

        std::cout << "==========================================" << std::endl;
        std::cout << "[*] Total Time for 10 Scans: " << std::fixed << std::setprecision(2)
            << total << " µs" << std::endl;
        std::cout << "[*] Average Time per Scan:  " << std::fixed << std::setprecision(2)
            << average << " µs" << std::endl;
        std::cout << "==========================================" << std::endl << std::endl;
    }
    return 0;
}