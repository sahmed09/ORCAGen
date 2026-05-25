#include <iostream>
#include <string>
#include <Windows.h>
#include <winreg.h>
#include <vector>  // Add this include for std::vector
#include <chrono>   // <--- Added

// Function to read registry values
bool ReadRegistryValue(HKEY hKey, const std::string& subKey, const std::string& valueName, std::string& value)
{
    HKEY hSubKey;
    LONG result = RegOpenKeyExA(hKey, subKey.c_str(), 0, KEY_READ, &hSubKey);

    if (result != ERROR_SUCCESS)
        return false;

    DWORD dwType = REG_SZ;
    char buffer[1024];
    DWORD dwSize = sizeof(buffer) - 1;

    result = RegQueryValueExA(hSubKey, valueName.c_str(), NULL, &dwType, (LPBYTE)buffer, &dwSize);

    if (result == ERROR_SUCCESS)
    {
        buffer[dwSize] = '\0';
        value = std::string(buffer);
    }
    else
    {
        value.clear();
    }

    RegCloseKey(hSubKey);
    return result == ERROR_SUCCESS;
}

// Function to read registry values on 64-bit systems
bool ReadRegistryValue64(HKEY hKey, const std::string& subKey, const std::string& valueName, std::string& value)
{
    HKEY hSubKey;
    LONG result = RegOpenKeyExA(hKey, subKey.c_str(), 0, KEY_READ | KEY_WOW64_64KEY, &hSubKey);

    if (result != ERROR_SUCCESS)
        return false;

    DWORD dwType = REG_SZ;
    char buffer[1024];
    DWORD dwSize = sizeof(buffer) - 1;

    result = RegQueryValueExA(hSubKey, valueName.c_str(), NULL, &dwType, (LPBYTE)buffer, &dwSize);

    if (result == ERROR_SUCCESS)
    {
        buffer[dwSize] = '\0';
        value = std::string(buffer);
    }
    else
    {
        value.clear();
    }

    RegCloseKey(hSubKey);
    return result == ERROR_SUCCESS;
}

// Function to read registry values on 32-bit systems
bool ReadRegistryValue32(HKEY hKey, const std::string& subKey, const std::string& valueName, std::string& value)
{
    HKEY hSubKey;
    LONG result = RegOpenKeyExA(hKey, subKey.c_str(), 0, KEY_READ | KEY_WOW64_32KEY, &hSubKey);

    if (result != ERROR_SUCCESS)
        return false;

    DWORD dwType = REG_SZ;
    char buffer[1024];
    DWORD dwSize = sizeof(buffer) - 1;

    result = RegQueryValueExA(hSubKey, valueName.c_str(), NULL, &dwType, (LPBYTE)buffer, &dwSize);

    if (result == ERROR_SUCCESS)
    {
        buffer[dwSize] = '\0';
        value = std::string(buffer);
    }
    else
    {
        value.clear();
    }

    RegCloseKey(hSubKey);
    return result == ERROR_SUCCESS;
}

// Function to mine Winlogon registry keys for credentials
void MineWinlogonRegistry()
{
    std::cout << "[*] Mining Winlogon registry keys for credentials...\n";

    // Registry key path
    const std::string winlogonKey = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon";

    // Try to read values from 64-bit registry first (most common)
    std::string defaultUserName, defaultPassword, autoAdminLogon;

    std::cout << "[+] Attempting to read from 64-bit registry...\n";
    ReadRegistryValue64(HKEY_LOCAL_MACHINE, winlogonKey, "DefaultUserName", defaultUserName);
    ReadRegistryValue64(HKEY_LOCAL_MACHINE, winlogonKey, "DefaultPassword", defaultPassword);
    ReadRegistryValue64(HKEY_LOCAL_MACHINE, winlogonKey, "AutoAdminLogon", autoAdminLogon);

    // If 64-bit fails, try 32-bit
    if (defaultUserName.empty() && defaultPassword.empty() && autoAdminLogon.empty())
    {
        std::cout << "[+] Falling back to 32-bit registry...\n";
        ReadRegistryValue32(HKEY_LOCAL_MACHINE, winlogonKey, "DefaultUserName", defaultUserName);
        ReadRegistryValue32(HKEY_LOCAL_MACHINE, winlogonKey, "DefaultPassword", defaultPassword);
        ReadRegistryValue32(HKEY_LOCAL_MACHINE, winlogonKey, "AutoAdminLogon", autoAdminLogon);
    }

    // Display results
    if (!defaultUserName.empty())
        std::cout << "[+] Default UserName: " << defaultUserName << "\n";
    else
        std::cout << "[-] Default UserName: Not found\n";

    if (!defaultPassword.empty())
        std::cout << "[+] Default Password: [HIDDEN - Password stored in registry]\n";
    else
        std::cout << "[-] Default Password: Not found\n";

    if (!autoAdminLogon.empty())
        std::cout << "[+] AutoAdminLogon: " << autoAdminLogon << "\n";
    else
        std::cout << "[-] AutoAdminLogon: Not found\n";
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

        std::cout << "Press <enter> to Beep (Ctrl-C to exit): ";
        std::getline(std::cin, value);

        // Mine registry again before each beep
        std::cout << "[*] Performing registry mining before Beep...\n";

        auto start = std::chrono::high_resolution_clock::now();
        MineWinlogonRegistry();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::micro> elapsed = end - start;

        std::cout << "[*] Registry Mining Complete.\n";
        std::cout << "[*] Runtime Overhead: " << elapsed.count() << " µs\n\n";

        Beep(500, 500); // Beep with frequency 500Hz for 500ms
    }

    return 0;
}