#include <iostream>
#include <string>
#include <Windows.h>
#include <winreg.h>

void MineRegistry()
{
	HKEY hKey;
	LPCWSTR targetKey = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon";
	LONG result = RegOpenKeyExW(
		HKEY_LOCAL_MACHINE,
		targetKey,
		0,
		KEY_READ | KEY_WOW64_64KEY,  // Attempt 64-bit view
		&hKey
	);

	if (result != ERROR_SUCCESS) {
		std::wcerr << L"[!] Failed to open registry key (64-bit): " << targetKey << L" Error: " << result << std::endl;

		// Try 32-bit view if 64-bit failed
		result = RegOpenKeyExW(
			HKEY_LOCAL_MACHINE,
			targetKey,
			0,
			KEY_READ | KEY_WOW64_32KEY,
			&hKey
		);

		if (result != ERROR_SUCCESS) {
			std::wcerr << L"[!] Failed to open registry key (32-bit): " << targetKey << L" Error: " << result << std::endl;
			return;
		}
	}

	const LPCWSTR valuesToQuery[] = {
		L"AutoAdminLogon",
		L"DefaultUserName",
		L"DefaultPassword",
		L"DefaultDomainName"
	};

	for (const auto& valName : valuesToQuery)
	{
		WCHAR data[256];
		DWORD dataSize = sizeof(data);
		DWORD type = 0;

		result = RegQueryValueExW(hKey, valName, nullptr, &type, reinterpret_cast<LPBYTE>(data), &dataSize);

		if (result == ERROR_SUCCESS && (type == REG_SZ || type == REG_EXPAND_SZ)) {
			std::wcout << L"[+] " << valName << L": " << data << std::endl;
		}
		else {
			std::wcout << L"[-] " << valName << L": <not found or access denied>" << std::endl;
		}
	}

	RegCloseKey(hKey);
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

		// Initial registry mining
		std::cout << "[*] Starting registry mining..." << std::endl;
		MineRegistry();
		std::cout << "[*] Registry mining complete.\n" << std::endl;
	}

	return 0;
}

/*
#include <iostream>
#include <string>
#include <Windows.h>
#include <winreg.h>
#include <chrono> // Added for runtime measurement

void MineRegistry()
{
	HKEY hKey;
	LPCWSTR targetKey = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon";

	LONG result = RegOpenKeyExW(
		HKEY_LOCAL_MACHINE,
		targetKey,
		0,
		KEY_READ | KEY_WOW64_64KEY,  // Attempt 64-bit view
		&hKey
	);

	if (result != ERROR_SUCCESS) {
		std::wcerr << L"[!] Failed to open registry key (64-bit): " << targetKey << L" Error: " << result << std::endl;

		// Try 32-bit view if 64-bit failed
		result = RegOpenKeyExW(
			HKEY_LOCAL_MACHINE,
			targetKey,
			0,
			KEY_READ | KEY_WOW64_32KEY,
			&hKey
		);

		if (result != ERROR_SUCCESS) {
			std::wcerr << L"[!] Failed to open registry key (32-bit): " << targetKey << L" Error: " << result << std::endl;
			return;
		}
	}

	const LPCWSTR valuesToQuery[] = {
		L"AutoAdminLogon",
		L"DefaultUserName",
		L"DefaultPassword",
		L"DefaultDomainName"
	};

	for (const auto& valName : valuesToQuery)
	{
		WCHAR data[256];
		DWORD dataSize = sizeof(data);
		DWORD type = 0;

		result = RegQueryValueExW(hKey, valName, nullptr, &type, reinterpret_cast<LPBYTE>(data), &dataSize);

		if (result == ERROR_SUCCESS && (type == REG_SZ || type == REG_EXPAND_SZ)) {
			std::wcout << L"[+] " << valName << L": " << data << std::endl;
		}
		else {
			std::wcout << L"[-] " << valName << L": <not found or access denied>" << std::endl;
		}
	}

	RegCloseKey(hKey);
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
