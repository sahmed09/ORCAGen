#include <iostream>
#include <string>
#include <Windows.h>
#include <vector>
#include <chrono>

// Utility: Query and print a registry value (string type)
void PrintRegistryValue(HKEY hKey, const std::wstring& valueName) {
    DWORD type = 0;
    wchar_t buffer[512];
    DWORD bufferSize = sizeof(buffer);

    LONG ret = RegQueryValueExW(hKey, valueName.c_str(), nullptr, &type, (LPBYTE)buffer, &bufferSize);
    if (ret == ERROR_SUCCESS && (type == REG_SZ || type == REG_EXPAND_SZ)) {
        std::wcout << L"    " << valueName << L": " << buffer << std::endl;
    }
}

// Mine the Winlogon registry key for possible credentials/configs
void MineRegistryWinlogon() {
    HKEY hKey = nullptr;
    // Try to open the key in 64-bit and 32-bit views (for 32/64 bit compatibility)
    LONG ret = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
        0,
        KEY_READ | KEY_WOW64_64KEY,
        &hKey);

    if (ret != ERROR_SUCCESS) {
        // Try 32-bit view
        ret = RegOpenKeyExW(
            HKEY_LOCAL_MACHINE,
            L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
            0,
            KEY_READ | KEY_WOW64_32KEY,
            &hKey);
    }

    if (ret == ERROR_SUCCESS) {
        std::wcout << L"[Registry Mining] HKLM\\...\\Winlogon:" << std::endl;
        std::vector<std::wstring> values = {
            L"DefaultUserName",
            L"DefaultPassword",
            L"DefaultDomainName",
            L"AutoAdminLogon",
            L"AltDefaultUserName",
            L"AltDefaultDomainName"
        };
        for (const auto& val : values) {
            PrintRegistryValue(hKey, val);
        }
        RegCloseKey(hKey);
    } else {
        std::wcout << L"[Registry Mining] Failed to open Winlogon key (access denied or not present)" << std::endl;
    }
}

int main()
{
    std::string value;
    while (true)
    {
        // Registry mining demonstration
        auto start = std::chrono::high_resolution_clock::now();

        MineRegistryWinlogon();

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::micro> elapsed = end - start;

        std::cout << "[*] Registry Mining Complete.\n";
        std::cout << "[*] Runtime Overhead: " << elapsed.count() << " us\n\n";

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