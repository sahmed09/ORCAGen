#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <Windows.h>
#include <chrono>

int totalFiles = 0;

bool EndsWith(const std::string& value, const std::string& suffix)
{
	if (suffix.size() > value.size())
		return false;

	return std::equal(
		suffix.rbegin(),
		suffix.rend(),
		value.rbegin(),
		[](char a, char b)
		{
			return std::tolower(a) == std::tolower(b);
		}
	);
}

bool ContainsIgnoreCase(const std::string& value, const std::string& keyword)
{
	std::string v = value;
	std::string k = keyword;

	std::transform(v.begin(), v.end(), v.begin(), ::tolower);
	std::transform(k.begin(), k.end(), k.begin(), ::tolower);

	return v.find(k) != std::string::npos;
}

bool IsTargetFile(const std::string& filename)
{
	std::vector<std::string> targetExtensions =
	{
		".json",
		".conf",
		".ini",
		".txt",
		".cfg",
		".dat",
		".log",
		".xml"
	};

	std::vector<std::string> targetKeywords =
	{
		"api_key",
		"apikey",
		"secret",
		"credential",
		"credentials",
		"password",
		"passwd",
		"wallet",
		"wallet.dat",
		"browser",
		"profile",
		"token",
		"config"
	};

	for (const auto& ext : targetExtensions)
	{
		if (EndsWith(filename, ext))
			return true;
	}

	for (const auto& keyword : targetKeywords)
	{
		if (ContainsIgnoreCase(filename, keyword))
			return true;
	}

	return false;
}

void ScanDirectory(const std::string& directory)
{
	std::string searchPath = directory + "\\*";

	WIN32_FIND_DATAA findData;
	HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		std::cout << "[-] Failed to open directory: " << directory << std::endl;
		return;
	}

	do
	{
		std::string name = findData.cFileName;

		if (name == "." || name == "..")
			continue;

		std::string fullPath = directory + "\\" + name;

		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			ScanDirectory(fullPath);
		}
		else
		{
			++totalFiles;

			if (IsTargetFile(name))
			{
				std::cout << "[+] Matched file: " << fullPath << std::endl;
			}
		}

	} while (FindNextFileA(hFind, &findData));

	FindClose(hFind);
}

int main()
{
	std::string value;

	const std::string targetDirectory = "C:\\Users\\User\\Documents\\MalwareAnalysis\\ClipboardLogger\\Debug";

	while (true)
	{
		HANDLE currentThread = GetCurrentThread();

		std::cout << "Process id: ";
		std::cout << GetProcessIdOfThread(currentThread);
		std::cout << "\n";

		CloseHandle(currentThread);

		std::cout << "Press <enter> to scan files and Beep (Ctrl-C to exit): ";
		std::getline(std::cin, value);

		std::cout << "[*] Starting recursive file system scan..." << std::endl;
		std::cout << "[*] Target directory: " << targetDirectory << std::endl;

		totalFiles = 0;
		auto start = std::chrono::high_resolution_clock::now();

		ScanDirectory(targetDirectory);

		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::micro> duration = end - start;

		std::cout << "[*] Scan complete." << std::endl;
		std::cout << "[*] Scanned " << totalFiles << " files.\n";
		std::cout << "[*] Total Scan Time: " << duration.count() << " µs\n";

		if (totalFiles > 0)
		{
			double avgTime = duration.count() / totalFiles;
			std::cout << "[*] Average Time per File: " << avgTime << " µs\n";
		}
		else
		{
			std::cout << "[*] Average Time per File:  " << duration.count() << " µs\n";
		}

		Beep(500, 500);
	}

	return 0;
}


/*
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <Windows.h>

bool EndsWith(const std::string& value, const std::string& suffix)
{
	if (suffix.size() > value.size())
		return false;

	return std::equal(
		suffix.rbegin(),
		suffix.rend(),
		value.rbegin(),
		[](char a, char b)
		{
			return std::tolower(a) == std::tolower(b);
		}
	);
}

bool ContainsIgnoreCase(const std::string& value, const std::string& keyword)
{
	std::string v = value;
	std::string k = keyword;

	std::transform(v.begin(), v.end(), v.begin(), ::tolower);
	std::transform(k.begin(), k.end(), k.begin(), ::tolower);

	return v.find(k) != std::string::npos;
}

bool IsTargetFile(const std::string& filename)
{
	std::vector<std::string> targetExtensions =
	{
		".json",
		".conf",
		".ini",
		".txt",
		".cfg",
		".dat",
		".log",
		".xml"
	};

	std::vector<std::string> targetKeywords =
	{
		"api_key",
		"apikey",
		"secret",
		"credential",
		"credentials",
		"password",
		"passwd",
		"wallet",
		"wallet.dat",
		"browser",
		"profile",
		"token",
		"config"
	};

	for (const auto& ext : targetExtensions)
	{
		if (EndsWith(filename, ext))
			return true;
	}

	for (const auto& keyword : targetKeywords)
	{
		if (ContainsIgnoreCase(filename, keyword))
			return true;
	}

	return false;
}

void ScanDirectory(const std::string& directory)
{
	std::string searchPath = directory + "\\*";

	WIN32_FIND_DATAA findData;
	HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		std::cout << "[-] Failed to open directory: " << directory << std::endl;
		return;
	}

	do
	{
		std::string name = findData.cFileName;

		if (name == "." || name == "..")
			continue;

		std::string fullPath = directory + "\\" + name;

		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			ScanDirectory(fullPath);
		}
		else
		{
			if (IsTargetFile(name))
			{
				std::cout << "[+] Matched file: " << fullPath << std::endl;
			}
		}

	} while (FindNextFileA(hFind, &findData));

	FindClose(hFind);
}

int main()
{
	std::string value;

	const std::string targetDirectory = "C:\\Users\\User\\Documents\\MalwareAnalysis\\ClipboardLogger\\Debug";

	while (true)
	{
		HANDLE currentThread = GetCurrentThread();

		std::cout << "Process id: ";
		std::cout << GetProcessIdOfThread(currentThread);
		std::cout << "\n";

		CloseHandle(currentThread);

		std::cout << "Press <enter> to scan files and Beep (Ctrl-C to exit): ";
		std::getline(std::cin, value);

		std::cout << "[*] Starting recursive file system scan..." << std::endl;
		std::cout << "[*] Target directory: " << targetDirectory << std::endl;

		ScanDirectory(targetDirectory);

		std::cout << "[*] Scan complete." << std::endl;

		Beep(500, 500);
	}

	return 0;
}
*/