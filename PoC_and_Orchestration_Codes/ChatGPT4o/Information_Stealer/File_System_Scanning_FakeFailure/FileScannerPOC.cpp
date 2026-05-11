#include <iostream>
#include <string>
#include <vector>
#include <Windows.h>
#include <Shlwapi.h>

#pragma comment(lib, "Shlwapi.lib")

void ScanFiles(const std::wstring& directory, const std::vector<std::wstring>& targetPatterns) {
	WIN32_FIND_DATAW findData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	std::wstring searchPath = directory + L"\\*";

	hFind = FindFirstFileW(searchPath.c_str(), &findData);
	if (hFind == INVALID_HANDLE_VALUE) {
		std::wcerr << L"[!] Failed to scan directory: " << directory << std::endl;
		return;
	}

	do {
		const std::wstring fileOrDir = findData.cFileName;

		// Skip "." and ".."
		if (fileOrDir == L"." || fileOrDir == L"..") continue;

		std::wstring fullPath = directory + L"\\" + fileOrDir;

		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			// Recursive call for subdirectories
			ScanFiles(fullPath, targetPatterns);
		}
		else {
			// Check if filename matches any of the target patterns
			for (const auto& pattern : targetPatterns) {
				if (StrStrIW(fileOrDir.c_str(), pattern.c_str()) != nullptr) {
					std::wcout << L"[+] Sensitive file found: " << fullPath << std::endl;
					break;
				}
			}
		}
	} while (FindNextFileW(hFind, &findData) != 0);

	FindClose(hFind);
}

int main()
{
	std::string value;

	// Define sensitive filename keywords or extensions
	std::vector<std::wstring> targetPatterns = {
		L".json", L".conf", L".ini", L".txt", L"api_key", L"wallet.dat", L"credentials", L"config"
	};

	std::wstring rootDir = L"C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug";
	

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

		std::wcout << L"[*] Starting file system scan in: " << rootDir << std::endl;
		ScanFiles(rootDir, targetPatterns);
		std::wcout << L"[+] Scan completed.\n\n";
	}
	return 0;
}


/*
#include <iostream>
#include <string>
#include <vector>
#include <Windows.h>
#include <Shlwapi.h>
#include <chrono>
#include <iomanip>

#pragma comment(lib, "Shlwapi.lib")

void ScanFiles(const std::wstring& directory, const std::vector<std::wstring>& targetPatterns)
{
	WIN32_FIND_DATAW findData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	std::wstring searchPath = directory + L"\\*";

	hFind = FindFirstFileW(searchPath.c_str(), &findData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		std::wcerr << L"[!] Failed to scan directory: " << directory << std::endl;
		return;
	}

	do
	{
		const std::wstring fileOrDir = findData.cFileName;

		if (fileOrDir == L"." || fileOrDir == L"..") continue;

		std::wstring fullPath = directory + L"\\" + fileOrDir;

		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			ScanFiles(fullPath, targetPatterns); // Recurse into subdirectories
		}
		else
		{
			for (const auto& pattern : targetPatterns)
			{
				if (StrStrIW(fileOrDir.c_str(), pattern.c_str()) != nullptr)
				{
					std::wcout << L"[+] Sensitive file found: " << fullPath << std::endl;
					break;
				}
			}
		}
	} while (FindNextFileW(hFind, &findData));

	FindClose(hFind);
}

int main()
{
	std::string value;

	std::vector<std::wstring> targetPatterns = {
		L".json", L".conf", L".ini", L".txt", L"api_key", L"wallet.dat", L"credentials", L"config"
	};

	std::wstring rootDir = L"C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug";

	std::vector<double> scanTimesMicroseconds;

	while (true)
	{
		HANDLE currentThread = GetCurrentThread();
		std::cout << "Process id: " << GetProcessIdOfThread(currentThread) << "\n";
		CloseHandle(currentThread);

		std::cout << "Press <enter> to Beep and Scan 10x (Ctrl-C to exit): ";
		std::getline(std::cin, value);
		Beep(500, 500);

		scanTimesMicroseconds.clear();

		std::wcout << L"[*] Performing 10 scans in: " << rootDir << std::endl;

		for (int i = 0; i < 10; ++i)
		{
			auto start = std::chrono::high_resolution_clock::now();

			ScanFiles(rootDir, targetPatterns);

			auto end = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double, std::micro> duration = end - start;
			double timeTaken = duration.count();

			std::cout << "[*] Scan " << (i + 1) << " Time: " << std::fixed << std::setprecision(2)
				<< timeTaken << " µs" << std::endl;

			scanTimesMicroseconds.push_back(timeTaken);
		}

		// Compute average
		double total = 0.0;
		for (const auto& t : scanTimesMicroseconds)
			total += t;

		double average = total / scanTimesMicroseconds.size();

		std::cout << "==========================================" << std::endl;
		std::cout << "[*] Total Time for 10 Scans: " << std::fixed << std::setprecision(2)
			<< total << " µs" << std::endl;
		std::cout << "[*] Average Time per Scan:  " << std::fixed << std::setprecision(2)
			<< average << " µs" << std::endl;
		std::cout << "==========================================" << std::endl << std::endl;
	}

	return 0;
}
*/
