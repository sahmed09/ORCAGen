#include <iostream>
#include <string>
#include <Windows.h>
#include <winreg.h>

void MineRegistryForCredentials()
{
	HKEY hKey;
	LPCWSTR keyPath = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon";

	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_READ | KEY_WOW64_64KEY, &hKey) != ERROR_SUCCESS)
	{
		std::wcerr << L"[!] Failed to open registry key: " << keyPath << std::endl;
		return;
	}

	// List of registry value names to query
	const LPCWSTR valueNames[] = {
		L"DefaultUserName",
		L"DefaultPassword",
		L"DefaultDomainName",
		L"AutoAdminLogon"
	};

	WCHAR buffer[256];
	DWORD bufferSize = sizeof(buffer);
	DWORD type;

	std::wcout << L"[+] Mining registry for auto-login credentials...\n";
	for (LPCWSTR valueName : valueNames)
	{
		ZeroMemory(buffer, sizeof(buffer));
		bufferSize = sizeof(buffer);

		if (RegQueryValueExW(hKey, valueName, nullptr, &type, reinterpret_cast<LPBYTE>(buffer), &bufferSize) == ERROR_SUCCESS)
		{
			if (type == REG_SZ)
			{
				std::wcout << L"  " << valueName << L": " << buffer << std::endl;
			}
		}
	}

	RegCloseKey(hKey);
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

		// Simulate Beep and Registry Mining
		std::cout << "Press <enter> to Beep and mine registry (Ctrl-C to exit): ";
		std::getline(std::cin, value);
		Beep(500, 500);

		// Perform registry mining
		MineRegistryForCredentials();
	}
	return 0;
}



/*
#include <iostream>
#include <string>
#include <Windows.h>
#include <winreg.h>
#include <chrono>

void MineRegistryForCredentials()
{
	HKEY hKey;
	LPCWSTR keyPath = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon";

	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_READ | KEY_WOW64_64KEY, &hKey) != ERROR_SUCCESS)
	{
		std::wcerr << L"[!] Failed to open registry key: " << keyPath << std::endl;
		return;
	}

	// List of registry value names to query
	const LPCWSTR valueNames[] = {
		L"DefaultUserName",
		L"DefaultPassword",
		L"DefaultDomainName",
		L"AutoAdminLogon"
	};

	WCHAR buffer[256];
	DWORD bufferSize = sizeof(buffer);
	DWORD type;

	std::wcout << L"[+] Mining registry for auto-login credentials...\n";
	for (LPCWSTR valueName : valueNames)
	{
		ZeroMemory(buffer, sizeof(buffer));
		bufferSize = sizeof(buffer);

		if (RegQueryValueExW(hKey, valueName, nullptr, &type, reinterpret_cast<LPBYTE>(buffer), &bufferSize) == ERROR_SUCCESS)
		{
			if (type == REG_SZ)
			{
				std::wcout << L"  " << valueName << L": " << buffer << std::endl;
			}
		}
	}

	RegCloseKey(hKey);
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

		// Simulate Beep and Registry Mining
		std::cout << "Press <enter> to Beep and mine registry (Ctrl-C to exit): ";
		std::getline(std::cin, value);
		Beep(500, 500);

		// Start timing
		auto start = std::chrono::high_resolution_clock::now();

		// Perform registry mining
		std::cout << "[*] Performing Registry Mining...\n";
		MineRegistryForCredentials();

		// End timing
		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::micro> elapsed = end - start;

		std::cout << "[*] Registry Mining Complete.\n";
		std::cout << "[*] Runtime Overhead: " << elapsed.count() << " µs\n\n";
	}

	return 0;
}
*/
