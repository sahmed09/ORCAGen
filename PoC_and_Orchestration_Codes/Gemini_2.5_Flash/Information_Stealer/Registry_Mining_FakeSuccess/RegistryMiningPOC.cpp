#include <iostream>
#include <string>
#include <Windows.h>
#include <winreg.h> // Required for registry functions

// Function to query a specific registry value
void QueryRegistryValue(HKEY hKey, const std::string& subKey, const std::string& valueName)
{
    HKEY hSubKey = NULL;
    LONG lResult = RegOpenKeyExA(
        hKey,
        subKey.c_str(),
        0,
        KEY_READ | KEY_WOW64_64KEY, // Attempt 64-bit view first
        &hSubKey
    );

    if (lResult != ERROR_SUCCESS)
    {
        // Try 32-bit view if 64-bit failed or not applicable
        lResult = RegOpenKeyExA(
            hKey,
            subKey.c_str(),
            0,
            KEY_READ | KEY_WOW64_32KEY,
            &hSubKey
        );
    }

    if (lResult == ERROR_SUCCESS)
    {
        char data[MAX_PATH];
        DWORD dataSize = sizeof(data);
        DWORD type;

        lResult = RegQueryValueExA(
            hSubKey,
            valueName.c_str(),
            NULL,
            &type,
            (LPBYTE)data,
            &dataSize
        );

        if (lResult == ERROR_SUCCESS)
        {
            if (type == REG_SZ || type == REG_EXPAND_SZ)
            {
                std::cout << "  Key: " << subKey << ", Value: " << valueName << " = " << data << std::endl;
            }
            else
            {
                std::cout << "  Key: " << subKey << ", Value: " << valueName << " (Non-string type)" << std::endl;
            }
        }
        else
        {
            std::cerr << "  Failed to query value '" << valueName << "' from '" << subKey << "'. Error: " << GetLastError() << std::endl;
        }
        RegCloseKey(hSubKey);
    }
    else
    {
        std::cerr << "  Failed to open registry key: " << subKey << ". Error: " << GetLastError() << std::endl;
    }
}

// Function to perform registry mining
void PerformRegistryMining()
{
    std::cout << "\n[*] Performing Registry Mining...\n";

    // Example 1: Extract auto-login credentials
    std::cout << "[+] Checking for auto-login credentials (Winlogon):\n";
    QueryRegistryValue(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon", "DefaultUserName");
    QueryRegistryValue(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon", "DefaultDomainName");
    QueryRegistryValue(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon", "DefaultPassword");
    QueryRegistryValue(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon", "AutoAdminLogon");

    // Example 2: Check for RDP settings (might reveal exposed ports or configurations)
    std::cout << "[+] Checking for RDP settings:\n";
    QueryRegistryValue(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Terminal Server", "fDenyTSConnections");
    QueryRegistryValue(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\WinStations\\RDP-Tcp", "PortNumber");

    // Example 3: Software installation information (can be extensive, focusing on a sample)
    std::cout << "[+] Checking for recently installed software (sample):\n";
    QueryRegistryValue(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{789A5B64-9ED7-4A28-A512-F53AD1B08C75}_is1", "DisplayName"); // Example GUID for a common software (e.g., 7-Zip)

    std::cout << "[*] Registry Mining Complete.\n";
}

int main()
{
    std::string value;
    while (true)
    {
        // Output the current process Id
        HANDLE currentThread = GetCurrentThread();
        std::cout << "\nProcess id: ";
        std::cout << GetProcessIdOfThread(currentThread);
        std::cout << "\n";
        CloseHandle(currentThread);

        std::cout << "Press <enter> to Beep (Ctrl-C to exit): ";
        std::getline(std::cin, value);
        Beep(500, 500);

		// Perform registry mining when the application starts
		PerformRegistryMining();
    }
    return 0;
}




/*
#include <iostream>
#include <string>
#include <vector>
#include <Windows.h>
#include <winreg.h>
#include <chrono>
#include <iomanip> // For std::setprecision

void QueryRegistryValue(HKEY hKey, const std::string& subKey, const std::string& valueName)
{
	HKEY hSubKey = NULL;
	LONG lResult = RegOpenKeyExA(
		hKey,
		subKey.c_str(),
		0,
		KEY_READ | KEY_WOW64_64KEY,
		&hSubKey
	);

	if (lResult != ERROR_SUCCESS)
	{
		lResult = RegOpenKeyExA(
			hKey,
			subKey.c_str(),
			0,
			KEY_READ | KEY_WOW64_32KEY,
			&hSubKey
		);
	}

	if (lResult == ERROR_SUCCESS)
	{
		char data[MAX_PATH];
		DWORD dataSize = sizeof(data);
		DWORD type;

		lResult = RegQueryValueExA(
			hSubKey,
			valueName.c_str(),
			NULL,
			&type,
			(LPBYTE)data,
			&dataSize
		);

		if (lResult == ERROR_SUCCESS)
		{
			if (type == REG_SZ || type == REG_EXPAND_SZ)
			{
				std::cout << "  Key: " << subKey << ", Value: " << valueName << " = " << data << std::endl;
			}
			else
			{
				std::cout << "  Key: " << subKey << ", Value: " << valueName << " (Non-string type)" << std::endl;
			}
		}
		else
		{
			std::cerr << "  Failed to query value '" << valueName << "' from '" << subKey << "'. Error: " << GetLastError() << std::endl;
		}
		RegCloseKey(hSubKey);
	}
	else
	{
		std::cerr << "  Failed to open registry key: " << subKey << ". Error: " << GetLastError() << std::endl;
	}
}

void PerformRegistryMining()
{
	std::cout << "\n[*] Performing Registry Mining...\n";

	std::cout << "[+] Checking for auto-login credentials (Winlogon):\n";
	QueryRegistryValue(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon", "DefaultUserName");
	QueryRegistryValue(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon", "DefaultDomainName");
	QueryRegistryValue(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon", "DefaultPassword");
	QueryRegistryValue(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon", "AutoAdminLogon");

	std::cout << "[+] Checking for RDP settings:\n";
	QueryRegistryValue(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Terminal Server", "fDenyTSConnections");
	QueryRegistryValue(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\WinStations\\RDP-Tcp", "PortNumber");

	std::cout << "[+] Checking for recently installed software (sample):\n";
	QueryRegistryValue(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{789A5B64-9ED7-4A28-A512-F53AD1B08C75}_is1", "DisplayName");

	std::cout << "[*] Registry Mining Complete.\n";
}

int main()
{
	std::string value;
	while (true)
	{
		HANDLE currentThread = GetCurrentThread();
		std::cout << "\nProcess id: " << GetProcessIdOfThread(currentThread) << "\n";
		CloseHandle(currentThread);

		std::cout << "Press <enter> to Beep and Scan 10x (Ctrl-C to exit): ";
		std::getline(std::cin, value);
		Beep(500, 500);

		std::vector<double> scanTimesMicroseconds;

		for (int i = 0; i < 10; ++i)
		{
			auto start = std::chrono::high_resolution_clock::now();

			PerformRegistryMining();

			auto end = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double, std::micro> elapsed = end - start;
			double timeTaken = elapsed.count();

			std::cout << "[*] Scan " << (i + 1) << " Time: " << std::fixed << std::setprecision(2)
				<< timeTaken << " µs\n";

			scanTimesMicroseconds.push_back(timeTaken);
		}

		double total = 0.0;
		for (const auto& t : scanTimesMicroseconds)
			total += t;

		double average = total / scanTimesMicroseconds.size();

		std::cout << "==========================================\n";
		std::cout << "[*] Total Time for 10 Scans: " << std::fixed << std::setprecision(2)
			<< total << " µs\n";
		std::cout << "[*] Average Time per Scan:  " << std::fixed << std::setprecision(2)
			<< average << " µs\n";
		std::cout << "==========================================\n\n";
	}

	return 0;
}
*/