#include <iostream>
#include <string>
#include <Windows.h>
#include <vector>
#include <iomanip>
#include <sstream>

// Function to encode a string to base64 (simplified implementation)
std::string base64_encode(const std::string& input) {
    static const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    std::string encoded;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    for (int idx = 0; idx < input.length(); idx++) {
        char_array_3[i++] = input[idx];
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; i < 4; i++) {
                if (char_array_4[i] < 64) {
                    encoded += base64_chars[char_array_4[i]];
                }
                else {
                    encoded += '=';
                }
            }
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 3; j++) {
            char_array_3[j] = '\0';
        }

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; j < i + 1; j++) {
            encoded += base64_chars[char_array_4[j]];
        }

        while ((i++ < 3)) {
            encoded += '=';
        }
    }

    return encoded;
}

// Function to get current user name
std::string get_current_user() {
    char username[256];
    DWORD username_len = 256;
    GetUserNameA(username, &username_len);
    return std::string(username);
}

// Function to rename files with encryption tags
void rename_encrypted_files(const std::string& target_directory) {
    // Create search pattern for all files in directory
    std::string search_path = target_directory + "\\*";

    WIN32_FIND_DATAA find_data;
    HANDLE hFind = FindFirstFileA(search_path.c_str(), &find_data);

    if (hFind == INVALID_HANDLE_VALUE) {
        std::cout << "No files found in directory: " << target_directory << std::endl;
        return;
    }

    // Generate a fake encryption key
    std::string victim_id = get_current_user();
    std::string base64_key = base64_encode(victim_id);

    do {
        // Skip directories and system files
        if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            std::string full_path = target_directory + "\\" + find_data.cFileName;

            // Get file extension
            std::string filename = find_data.cFileName;
            size_t dot_pos = filename.find_last_of('.');

            // Create new filename with encryption tag
            std::string new_filename;
            if (dot_pos != std::string::npos) {
                // Append encryption tag to existing extension
                std::string base_name = filename.substr(0, dot_pos);
                std::string extension = filename.substr(dot_pos);

                // Add encryption tag with base64 encoded key
                new_filename = base_name + ".encrypted." + base64_key;
            }
            else {
                // No extension, just append the tag
                new_filename = filename + ".encrypted." + base64_key;
            }

            std::string new_full_path = target_directory + "\\" + new_filename;

            // Rename file using MoveFileEx
            if (MoveFileExA(full_path.c_str(), new_full_path.c_str(), MOVEFILE_REPLACE_EXISTING)) {
                std::cout << "[+] File renamed: " << find_data.cFileName << " -> " << new_filename << std::endl;
            }
            else {
                std::cout << "[-] Failed to rename file: " << find_data.cFileName << std::endl;
            }
        }
    } while (FindNextFileA(hFind, &find_data));

    FindClose(hFind);
}

// Function to create test files in the directory
void create_test_files(const std::string& target_directory) {
    // Ensure directory exists
    CreateDirectoryA(target_directory.c_str(), NULL);

    // Create some sample files
    const char* filenames[] = { "test1.txt", "document.pdf", "image.jpg", "data.xml" };

    for (const auto& filename : filenames) {
        std::string full_path = target_directory + "\\" + filename;

        HANDLE hFile = CreateFileA(
            full_path.c_str(),
            GENERIC_WRITE,
            0,
            NULL,
            CREATE_NEW,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );

        if (hFile != INVALID_HANDLE_VALUE) {
            // Write some sample content
            std::string content = "This is a test file for ransomware simulation.";
            DWORD bytes_written;
            WriteFile(hFile, content.c_str(), content.length(), &bytes_written, NULL);
            CloseHandle(hFile);
            std::cout << "[+] Created test file: " << filename << std::endl;
        }
    }
}

int main()
{
    std::string value;

    // Define the target directory
    std::string target_directory = "C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug\\Files";

    // Create test files for demonstration
    create_test_files(target_directory);

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

        // Add file renaming functionality after each beep
        std::cout << "[*] Executing file renaming operation..." << std::endl;
        rename_encrypted_files(target_directory);
        std::cout << "[*] File renaming complete." << std::endl;
    }

    return 0;
}



/*
#include <iostream>
#include <string>
#include <Windows.h>
#include <vector>
#include <iomanip>
#include <sstream>
#include <chrono>

// Function to encode a string to base64 (simplified)
std::string base64_encode(const std::string& input) {
    static const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string encoded;
    int i = 0, j = 0;
    unsigned char char_array_3[3], char_array_4[4];

    for (int idx = 0; idx < input.length(); idx++) {
        char_array_3[i++] = input[idx];
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; i < 4; i++)
                encoded += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 3; j++) char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; j < i + 1; j++) encoded += base64_chars[char_array_4[j]];
        while ((i++ < 3)) encoded += '=';
    }

    return encoded;
}

// Get current user name
std::string get_current_user() {
    char username[256];
    DWORD username_len = 256;
    GetUserNameA(username, &username_len);
    return std::string(username);
}

// Rename files and calculate rename timing
void rename_encrypted_files(const std::string& target_directory) {
    std::string search_path = target_directory + "\\*";
    WIN32_FIND_DATAA find_data;
    HANDLE hFind = FindFirstFileA(search_path.c_str(), &find_data);

    if (hFind == INVALID_HANDLE_VALUE) {
        std::cout << "No files found in directory: " << target_directory << std::endl;
        return;
    }

    std::string victim_id = get_current_user();
    std::string base64_key = base64_encode(victim_id);

    int attempt_count = 0;
    double total_microseconds = 0.0;

    do {
        if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            std::string full_path = target_directory + "\\" + find_data.cFileName;
            std::string filename = find_data.cFileName;
            size_t dot_pos = filename.find_last_of('.');

            std::string new_filename;
            if (dot_pos != std::string::npos) {
                std::string base_name = filename.substr(0, dot_pos);
                new_filename = base_name + ".encrypted." + base64_key;
            }
            else {
                new_filename = filename + ".encrypted." + base64_key;
            }

            std::string new_full_path = target_directory + "\\" + new_filename;

            auto start = std::chrono::high_resolution_clock::now();
            BOOL result = MoveFileExA(full_path.c_str(), new_full_path.c_str(), MOVEFILE_REPLACE_EXISTING);
            auto end = std::chrono::high_resolution_clock::now();

            std::chrono::duration<double, std::micro> duration = end - start;
            total_microseconds += duration.count();
            ++attempt_count;

            if (result) {
                std::cout << "[+] File renamed: " << filename << " -> " << new_filename
                    << " (Time: " << duration.count() << " µs)\n";
            }
            else {
                std::cout << "[-] Failed to rename file: " << filename
                    << " (Time: " << duration.count() << " µs)\n";
            }
        }
    } while (FindNextFileA(hFind, &find_data));

    FindClose(hFind);

    if (attempt_count > 0) {
        double avg = total_microseconds / attempt_count;
        std::cout << "[*] Total files attempted: " << attempt_count << "\n";
        std::cout << "[*] Total time spent: " << total_microseconds << " µs\n";
        std::cout << "[*] Average time per attempt: " << avg << " µs\n";
    }
    else {
        std::cout << "[!] No files to rename.\n";
    }
}

// Create some test files
void create_test_files(const std::string& target_directory) {
    CreateDirectoryA(target_directory.c_str(), NULL);

    const char* filenames[] = { "test1.txt", "document.pdf", "image.jpg", "data.xml" };

    for (const auto& filename : filenames) {
        std::string full_path = target_directory + "\\" + filename;
        HANDLE hFile = CreateFileA(full_path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

        if (hFile != INVALID_HANDLE_VALUE) {
            std::string content = "Sample ransomware simulation content.";
            DWORD bytes_written;
            WriteFile(hFile, content.c_str(), content.length(), &bytes_written, NULL);
            CloseHandle(hFile);
            std::cout << "[+] Created test file: " << filename << std::endl;
        }
    }
}

int main() {
    std::string value;
    std::string target_directory = "C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug\\Files";

    create_test_files(target_directory);

    while (true) {
        HANDLE currentThread = GetCurrentThread();
        std::cout << "Process id: " << GetProcessIdOfThread(currentThread) << "\n";
        CloseHandle(currentThread);

        std::cout << "Press <enter> to rename files (Ctrl-C to exit): ";
        std::getline(std::cin, value);

        Beep(500, 500);
        std::cout << "[*] Starting file renaming with timing...\n";
        rename_encrypted_files(target_directory);
        std::cout << "[*] File renaming complete.\n";
    }

    return 0;
}
*/