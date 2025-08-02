#include <iostream>
#include <string>
#include <Windows.h>
#include <vector>

// Helper function to query and print a registry value
void PrintRegValue(HKEY hKey, const wchar_t* valueName) {
	DWORD type = 0;
	wchar_t data[512];
	DWORD dataSize = sizeof(data);

	LONG res = RegQueryValueExW(hKey, valueName, NULL, &type, reinterpret_cast<LPBYTE>(data), &dataSize);
	if (res == ERROR_SUCCESS && (type == REG_SZ || type == REG_EXPAND_SZ)) {
		std::wcout << L"    " << valueName << L": " << data << std::endl;
	}
}

// Registry mining function
void MineRegistry() {
	std::wcout << L"[Registry Mining] Attempting to extract autologon credentials...\n";

	HKEY hKey = nullptr;
	// Try to open the Winlogon key with 64-bit and 32-bit views
	std::vector<DWORD> views = { KEY_WOW64_64KEY, KEY_WOW64_32KEY };
	for (DWORD view : views) {
		LONG res = RegOpenKeyExW(
			HKEY_LOCAL_MACHINE,
			L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
			0,
			KEY_READ | view,
			&hKey
		);

		if (res == ERROR_SUCCESS) {
			std::wcout << L"[+] Opened Winlogon key (" << ((view == KEY_WOW64_64KEY) ? L"64-bit" : L"32-bit") << L" view):\n";
			PrintRegValue(hKey, L"DefaultUserName");
			PrintRegValue(hKey, L"DefaultPassword");
			PrintRegValue(hKey, L"DefaultDomainName");
			PrintRegValue(hKey, L"AltDefaultUserName");
			PrintRegValue(hKey, L"AltDefaultDomainName");
			PrintRegValue(hKey, L"AutoAdminLogon");
			RegCloseKey(hKey);
		}
		else {
			// Only print failure if both fail at end
			continue;
		}
	}
	std::wcout << L"[Registry Mining] Done.\n";
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

		// 1. Mine registry for credential data
		MineRegistry();
	}
	return 0;
}



/*
#include <iostream>
#include <string>
#include <Windows.h>
#include <vector>
#include <chrono> // Include for timing

// Helper function to query and print a registry value
void PrintRegValue(HKEY hKey, const wchar_t* valueName) {
	DWORD type = 0;
	wchar_t data[512];
	DWORD dataSize = sizeof(data);

	LONG res = RegQueryValueExW(hKey, valueName, NULL, &type, reinterpret_cast<LPBYTE>(data), &dataSize);
	if (res == ERROR_SUCCESS && (type == REG_SZ || type == REG_EXPAND_SZ)) {
		std::wcout << L"    " << valueName << L": " << data << std::endl;
	}
}

// Registry mining function
void MineRegistry() {
	std::wcout << L"[Registry Mining] Attempting to extract autologon credentials...\n";

	HKEY hKey = nullptr;
	// Try to open the Winlogon key with 64-bit and 32-bit views
	std::vector<DWORD> views = { KEY_WOW64_64KEY, KEY_WOW64_32KEY };
	for (DWORD view : views) {
		LONG res = RegOpenKeyExW(
			HKEY_LOCAL_MACHINE,
			L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
			0,
			KEY_READ | view,
			&hKey
		);

		if (res == ERROR_SUCCESS) {
			std::wcout << L"[+] Opened Winlogon key (" << ((view == KEY_WOW64_64KEY) ? L"64-bit" : L"32-bit") << L" view):\n";
			PrintRegValue(hKey, L"DefaultUserName");
			PrintRegValue(hKey, L"DefaultPassword");
			PrintRegValue(hKey, L"DefaultDomainName");
			PrintRegValue(hKey, L"AltDefaultUserName");
			PrintRegValue(hKey, L"AltDefaultDomainName");
			PrintRegValue(hKey, L"AutoAdminLogon");
			RegCloseKey(hKey);
		}
		else {
			// Only print failure if both fail at end
			continue;
		}
	}
	std::wcout << L"[Registry Mining] Done.\n";
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
		MineRegistry();
		auto end = std::chrono::high_resolution_clock::now();

		std::chrono::duration<double, std::micro> elapsed = end - start;

		std::cout << "[*] Registry mining complete." << std::endl;
		std::cout << "[*] Runtime Overhead: " << elapsed.count() << " µs\n" << std::endl;
	}
	return 0;
}
*/