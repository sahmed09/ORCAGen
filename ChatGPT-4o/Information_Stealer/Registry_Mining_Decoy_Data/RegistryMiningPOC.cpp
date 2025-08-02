#include <iostream>
#include <string>
#include <Windows.h>
#include <winreg.h>
#include <chrono>

void QueryWinlogonCredentials()
{
	HKEY hKey;
	LPCWSTR subKey = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon";

	// Attempt to open the registry key
	LONG status = RegOpenKeyExW(
		HKEY_LOCAL_MACHINE,
		subKey,
		0,
		KEY_READ | KEY_WOW64_64KEY, // Support both 32-bit and 64-bit views
		&hKey
	);

	if (status != ERROR_SUCCESS)
	{
		std::cerr << "[!] Failed to open registry key. Error: " << status << std::endl;
		return;
	}

	// List of target value names to extract
	const wchar_t* valueNames[] = { L"DefaultUserName", L"DefaultPassword", L"AutoAdminLogon" };

	for (const auto& valueName : valueNames)
	{
		WCHAR data[256];
		DWORD dataSize = sizeof(data);
		DWORD type = 0;

		status = RegQueryValueExW(
			hKey,
			valueName,
			nullptr,
			&type,
			reinterpret_cast<LPBYTE>(data),
			&dataSize
		);

		if (status == ERROR_SUCCESS && (type == REG_SZ || type == REG_EXPAND_SZ))
		{
			std::wcout << L"[Registry] " << valueName << L" = " << data << std::endl;
		}
		else
		{
			std::wcout << L"[Registry] " << valueName << L" not found or unreadable." << std::endl;
		}
	}

	RegCloseKey(hKey);
}


int main()
{
	std::string value;

	// --- Simulated Clipboard Logger Loop ---
	while (true)
	{
		HANDLE currentThread = GetCurrentThread();
		std::cout << "Process id: ";
		std::cout << GetProcessIdOfThread(currentThread);
		std::cout << "\n";
		CloseHandle(currentThread);

		std::cout << "Press <enter> to Beep (Ctrl-C to exit): ";
		std::getline(std::cin, value);
		Beep(500, 500);

		// --- Registry Mining Execution ---
		std::cout << "[*] Performing Registry Mining...\n";

		// Start timing
		auto start = std::chrono::high_resolution_clock::now();

		QueryWinlogonCredentials();
		std::cout << "[*] Registry Mining Complete.\n\n";

		// End timing
		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::micro> elapsed = end - start;

		std::cout << "[*] Registry Mining Complete.\n";
		std::cout << "[*] Runtime Overhead: " << elapsed.count() << " µs\n\n";
	}

	return 0;
}
