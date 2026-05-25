#include <iostream>
#include <string>
#include <vector>
#include <Windows.h>
#include <chrono>

static const std::wstring ENCRYPTED_TAG = L".locked";

// Extensions to simulate “encrypted file” targeting
static const std::vector<std::wstring> TARGET_EXTENSIONS = {
    L".txt", L".docx", L".xlsx", L".pdf", L".db"
};

int attemptCount = 0;
double totalMicroseconds = 0.0;

bool EndsWith(const std::wstring& value, const std::wstring& suffix)
{
    if (suffix.size() > value.size())
        return false;

    return _wcsicmp(
        value.c_str() + value.size() - suffix.size(),
        suffix.c_str()
    ) == 0;
}

bool IsTargetFile(const std::wstring& fileName)
{
    if (EndsWith(fileName, ENCRYPTED_TAG))
        return false;

    for (const auto& ext : TARGET_EXTENSIONS)
    {
        if (EndsWith(fileName, ext))
            return true;
    }

    return false;
}

void RenameFileWithEncryptedExtension(const std::wstring& filePath)
{
    std::wstring newPath = filePath + ENCRYPTED_TAG;

    auto start = std::chrono::high_resolution_clock::now();

    BOOL result = MoveFileExW(
        filePath.c_str(),
        newPath.c_str(),
        MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH
    );

    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double, std::micro> duration = end - start;
    totalMicroseconds += duration.count();
    ++attemptCount;

    if (result)
    {
        std::wcout << L"[+] Renamed: " << filePath << L"\n";
        std::wcout << L"    -> " << newPath << L"\n";
        std::wcout << L"    Time: " << duration.count() << L" µs\n";
    }
    else
    {
        std::wcout << L"[-] Rename failed: " << filePath
            << L" | Error: " << GetLastError()
            << L" | Time: " << duration.count() << L" µs\n";
    }
}

void EnumerateAndRenameFiles(const std::wstring& directory)
{
    std::wstring searchPath = directory + L"\\*";

    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        std::wcout << L"[-] Could not open directory: " << directory
            << L" | Error: " << GetLastError() << L"\n";
        return;
    }

    do
    {
        std::wstring name = findData.cFileName;

        if (name == L"." || name == L"..")
            continue;

        std::wstring fullPath = directory + L"\\" + name;

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            EnumerateAndRenameFiles(fullPath);
        }
        else
        {
            if (IsTargetFile(name))
            {
                RenameFileWithEncryptedExtension(fullPath);
            }
        }

    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);
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

        std::cout << "\nOptions:\n";
        std::cout << "1. Press <enter> to Beep\n";
        std::cout << "2. Type rename to simulate extension-based encryption marking\n";
        std::cout << "3. Ctrl-C to exit\n";
        std::cout << "> ";

        std::getline(std::cin, value);

        if (value == "rename")
        {
            std::wstring targetDir;

            std::wcout << L"Enter test directory path: ";
            std::getline(std::wcin, targetDir);

            if (!targetDir.empty())
            {
                std::wcout << L"[*] Starting file rename simulation in: "
                    << targetDir << L"\n";

                attemptCount = 0;
                totalMicroseconds = 0.0;

                EnumerateAndRenameFiles(targetDir);

                std::wcout << L"[*] Rename simulation complete.\n";

                if (attemptCount > 0)
                {
                    double avgTime = totalMicroseconds / attemptCount;
                    std::wcout << L"[*] Total files attempted: " << attemptCount << L"\n";
                    std::wcout << L"[*] Total time spent: " << totalMicroseconds << L" µs\n";
                    std::wcout << L"[*] Average time per attempt: " << avgTime << L" µs\n";
                }
                else
                {
                    std::wcout << L"[!] No target files found for encryption.\n";
                }
            }
            else
            {
                std::wcout << L"[-] Empty directory path provided.\n";
            }
        }
        else
        {
            Beep(500, 500);
        }
    }

    return 0;
}


/*
#include <iostream>
#include <string>
#include <vector>
#include <Windows.h>

static const std::wstring ENCRYPTED_TAG = L".locked";

// Extensions to simulate “encrypted file” targeting
static const std::vector<std::wstring> TARGET_EXTENSIONS = {
    L".txt", L".docx", L".xlsx", L".pdf", L".db"
};

bool EndsWith(const std::wstring& value, const std::wstring& suffix)
{
    if (suffix.size() > value.size())
        return false;

    return _wcsicmp(
        value.c_str() + value.size() - suffix.size(),
        suffix.c_str()
    ) == 0;
}

bool IsTargetFile(const std::wstring& fileName)
{
    if (EndsWith(fileName, ENCRYPTED_TAG))
        return false;

    for (const auto& ext : TARGET_EXTENSIONS)
    {
        if (EndsWith(fileName, ext))
            return true;
    }

    return false;
}

void RenameFileWithEncryptedExtension(const std::wstring& filePath)
{
    std::wstring newPath = filePath + ENCRYPTED_TAG;

    BOOL result = MoveFileExW(
        filePath.c_str(),
        newPath.c_str(),
        MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH
    );

    if (result)
    {
        std::wcout << L"[+] Renamed: " << filePath << L"\n";
        std::wcout << L"    -> " << newPath << L"\n";
    }
    else
    {
        std::wcout << L"[-] Rename failed: " << filePath
            << L" | Error: " << GetLastError() << L"\n";
    }
}

void EnumerateAndRenameFiles(const std::wstring& directory)
{
    std::wstring searchPath = directory + L"\\*";

    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        std::wcout << L"[-] Could not open directory: " << directory
            << L" | Error: " << GetLastError() << L"\n";
        return;
    }

    do
    {
        std::wstring name = findData.cFileName;

        if (name == L"." || name == L"..")
            continue;

        std::wstring fullPath = directory + L"\\" + name;

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            EnumerateAndRenameFiles(fullPath);
        }
        else
        {
            if (IsTargetFile(name))
            {
                RenameFileWithEncryptedExtension(fullPath);
            }
        }

    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);
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

        std::cout << "\nOptions:\n";
        std::cout << "1. Press <enter> to Beep\n";
        std::cout << "2. Type rename to simulate extension-based encryption marking\n";
        std::cout << "3. Ctrl-C to exit\n";
        std::cout << "> ";

        std::getline(std::cin, value);

        if (value == "rename")
        {
            std::wstring targetDir;

            std::wcout << L"Enter test directory path: ";
            std::getline(std::wcin, targetDir);

            if (!targetDir.empty())
            {
                std::wcout << L"[*] Starting file rename simulation in: "
                    << targetDir << L"\n";

                EnumerateAndRenameFiles(targetDir);

                std::wcout << L"[*] Rename simulation complete.\n";
            }
            else
            {
                std::wcout << L"[-] Empty directory path provided.\n";
            }
        }
        else
        {
            Beep(500, 500);
        }
    }

    return 0;
}
*/