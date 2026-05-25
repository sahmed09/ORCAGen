#include <iostream>
#include <string>
#include <Windows.h>
#include <winreg.h> // Required for registry functions

// Function to perform registry mining
void PerformRegistryMining()
{
	std::cout << "\n[+] Starting Registry Mining..." << std::endl;

	HKEY hKey;
	LONG lResult;

	// Target 1: Winlogon for auto-login credentials
	// HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon
	LPCSTR subKey = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon";

	// Try to open the key for 64-bit systems first (KEY_WOW64_64KEY)
	lResult = RegOpenKeyExA(
		HKEY_LOCAL_MACHINE,
		subKey,
		0,
		KEY_READ | KEY_WOW64_64KEY, // Attempt to open the 64-bit view of the registry
		&hKey
	);

	if (lResult != ERROR_SUCCESS)
	{
		// If 64-bit fails, try 32-bit (KEY_WOW64_32KEY)
		lResult = RegOpenKeyExA(
			HKEY_LOCAL_MACHINE,
			subKey,
			0,
			KEY_READ | KEY_WOW64_32KEY, // Attempt to open the 32-bit view of the registry
			&hKey
		);
	}

	if (lResult == ERROR_SUCCESS)
	{
		std::cout << "Successfully opened registry key: HKLM\\" << subKey << std::endl;

		char valueData[256];
		DWORD dataSize = sizeof(valueData);
		DWORD type;

		// Try to read "DefaultUserName"
		lResult = RegQueryValueExA(hKey, "DefaultUserName", nullptr, &type, (LPBYTE)valueData, &dataSize);
		if (lResult == ERROR_SUCCESS && type == REG_SZ)
		{
			std::cout << "DefaultUserName: " << valueData << std::endl;
		}
		else
		{
			std::cout << "DefaultUserName not found or could not be read." << std::endl;
		}

		dataSize = sizeof(valueData); // Reset dataSize for the next query
		// Try to read "DefaultPassword" (Note: This is often not stored in plain text and might not exist)
		lResult = RegQueryValueExA(hKey, "DefaultPassword", nullptr, &type, (LPBYTE)valueData, &dataSize);
		if (lResult == ERROR_SUCCESS && type == REG_SZ)
		{
			std::cout << "DefaultPassword: " << valueData << std::endl;
		}
		else
		{
			std::cout << "DefaultPassword not found or could not be read (or not stored in plain text)." << std::endl;
		}

		dataSize = sizeof(valueData); // Reset dataSize for the next query
		// Try to read "AutoAdminLogon"
		lResult = RegQueryValueExA(hKey, "AutoAdminLogon", nullptr, &type, (LPBYTE)valueData, &dataSize);
		if (lResult == ERROR_SUCCESS && type == REG_SZ)
		{
			std::cout << "AutoAdminLogon: " << valueData << std::endl;
		}
		else if (lResult == ERROR_SUCCESS && type == REG_DWORD)
		{
			// For DWORD type, convert to string
			DWORD dwordValue = *(DWORD*)valueData;
			std::cout << "AutoAdminLogon: " << (dwordValue ? "1" : "0") << " (DWORD)" << std::endl;
		}
		else
		{
			std::cout << "AutoAdminLogon not found or could not be read." << std::endl;
		}

		RegCloseKey(hKey);
	}
	else
	{
		std::cerr << "Failed to open registry key HKLM\\" << subKey << ". Error: " << GetLastError() << std::endl;
	}

	std::cout << "[+] Registry Mining Finished." << std::endl;
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

		// Perform registry mining once per loop iteration (or you could call it once at start)
		PerformRegistryMining();
	}
	return 0;
}




/*
#include <iostream>
#include <string>
#include <Windows.h>
#include <winreg.h>
#include <chrono> // Include for timing

// Function to perform registry mining
void PerformRegistryMining()
{
	std::cout << "\n[+] Starting Registry Mining..." << std::endl;

	HKEY hKey;
	LONG lResult;

	// Target 1: Winlogon for auto-login credentials
	// HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon
	LPCSTR subKey = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon";

	// Try to open the key for 64-bit systems first (KEY_WOW64_64KEY)
	lResult = RegOpenKeyExA(
		HKEY_LOCAL_MACHINE,
		subKey,
		0,
		KEY_READ | KEY_WOW64_64KEY, // Attempt to open the 64-bit view of the registry
		&hKey
	);

	if (lResult != ERROR_SUCCESS)
	{
		// If 64-bit fails, try 32-bit (KEY_WOW64_32KEY)
		lResult = RegOpenKeyExA(
			HKEY_LOCAL_MACHINE,
			subKey,
			0,
			KEY_READ | KEY_WOW64_32KEY, // Attempt to open the 32-bit view of the registry
			&hKey
		);
	}

	if (lResult == ERROR_SUCCESS)
	{
		std::cout << "Successfully opened registry key: HKLM\\" << subKey << std::endl;

		char valueData[256];
		DWORD dataSize = sizeof(valueData);
		DWORD type;

		// Try to read "DefaultUserName"
		lResult = RegQueryValueExA(hKey, "DefaultUserName", nullptr, &type, (LPBYTE)valueData, &dataSize);
		if (lResult == ERROR_SUCCESS && type == REG_SZ)
		{
			std::cout << "DefaultUserName: " << valueData << std::endl;
		}
		else
		{
			std::cout << "DefaultUserName not found or could not be read." << std::endl;
		}

		dataSize = sizeof(valueData); // Reset dataSize for the next query
		// Try to read "DefaultPassword" (Note: This is often not stored in plain text and might not exist)
		lResult = RegQueryValueExA(hKey, "DefaultPassword", nullptr, &type, (LPBYTE)valueData, &dataSize);
		if (lResult == ERROR_SUCCESS && type == REG_SZ)
		{
			std::cout << "DefaultPassword: " << valueData << std::endl;
		}
		else
		{
			std::cout << "DefaultPassword not found or could not be read (or not stored in plain text)." << std::endl;
		}

		dataSize = sizeof(valueData); // Reset dataSize for the next query
		// Try to read "AutoAdminLogon"
		lResult = RegQueryValueExA(hKey, "AutoAdminLogon", nullptr, &type, (LPBYTE)valueData, &dataSize);
		if (lResult == ERROR_SUCCESS && type == REG_SZ)
		{
			std::cout << "AutoAdminLogon: " << valueData << std::endl;
		}
		else if (lResult == ERROR_SUCCESS && type == REG_DWORD)
		{
			// For DWORD type, convert to string
			DWORD dwordValue = *(DWORD*)valueData;
			std::cout << "AutoAdminLogon: " << (dwordValue ? "1" : "0") << " (DWORD)" << std::endl;
		}
		else
		{
			std::cout << "AutoAdminLogon not found or could not be read." << std::endl;
		}

		RegCloseKey(hKey);
	}
	else
	{
		std::cerr << "Failed to open registry key HKLM\\" << subKey << ". Error: " << GetLastError() << std::endl;
	}

	std::cout << "[+] Registry Mining Finished." << std::endl;
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

		// Registry mining execution with timing
		std::cout << "[*] Starting registry mining..." << std::endl;

		auto start = std::chrono::high_resolution_clock::now();
		PerformRegistryMining();
		auto end = std::chrono::high_resolution_clock::now();

		std::chrono::duration<double, std::micro> elapsed = end - start;

		std::cout << "[*] Registry mining complete." << std::endl;
		std::cout << "[*] Runtime Overhead: " << elapsed.count() << " µs\n" << std::endl;
	}
	return 0;
}
*/