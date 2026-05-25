#include <iostream>
#include <string>
#include <vector>
#include <Windows.h>
#include <winreg.h>
#include <chrono>

void MineRegistry()
{
    HKEY hKey;
    // Target registry key for Winlogon auto-login credentials
    LPCWSTR subkey = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon";
    // Typical value names to extract
    std::vector<std::wstring> valueNames = {
        L"DefaultUserName",
        L"DefaultPassword",
        L"AltDefaultUserName",
        L"DefaultDomainName",
        L"AutoAdminLogon"
    };

    LONG result = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        subkey,
        0,
        KEY_READ | KEY_WOW64_64KEY, // Handles 64-bit registry view
        &hKey
    );

    if (result != ERROR_SUCCESS) {
        std::cout << "[!] Failed to open registry key (error " << result << ")\n";
        return;
    }

    std::wcout << L"[+] Registry Mining: " << subkey << std::endl;

    for (const auto& valName : valueNames) {
        WCHAR data[256];
        DWORD dataSize = sizeof(data);
        DWORD type = 0;

        result = RegQueryValueExW(
            hKey,
            valName.c_str(),
            nullptr,
            &type,
            reinterpret_cast<LPBYTE>(data),
            &dataSize
        );

        if (result == ERROR_SUCCESS && (type == REG_SZ || type == REG_EXPAND_SZ)) {
            data[(dataSize / sizeof(WCHAR)) - 1] = L'\0'; // Ensure null termination
            std::wcout << L"    " << valName << L": " << data << std::endl;
        }
        else {
            std::wcout << L"    " << valName << L": <not found or error>" << std::endl;
        }
    }
    RegCloseKey(hKey);
    std::wcout << std::endl;
}

int main()
{
    std::string value;
    while (true)
    {
        // Output the current process Id
        HANDLE currentThread = GetCurrentThread();
        std::cout << "Process id: ";
        std::cout << GetProcessIdOfThread(currentThread);
        std::cout << "\n";
        CloseHandle(currentThread);

        // --- Registry Mining ---
        auto start = std::chrono::high_resolution_clock::now();

        MineRegistry();

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::micro> elapsed = end - start;

        std::cout << "[*] Registry Mining Complete.\n";
        std::cout << "[*] Runtime Overhead: " << elapsed.count() << " us\n\n";

        std::cout << "Press <enter> to Beep (Ctrl-C to exit): ";
        std::getline(std::cin, value);
        Beep(500, 500);
    }
    return 0;
}