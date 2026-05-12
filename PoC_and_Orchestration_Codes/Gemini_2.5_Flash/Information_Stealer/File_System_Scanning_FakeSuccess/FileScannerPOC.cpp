#include <iostream>
#include <string>
#include <vector>
#include <Windows.h>
#include <Shlwapi.h> // For PathCombine, requires linking with Shlwapi.lib
#include <algorithm> // For std::transform
#include <cctype>    // For ::tolower (and in a pinch, ::towlower often works from here)
#include <cwctype>   // For ::towlower (more specifically for wide characters)

// Link with Shlwapi.lib
#pragma comment(lib, "Shlwapi.lib")

// List of sensitive file extensions and filenames to look for
const std::vector<std::wstring> sensitiveExtensions = {
	L".json", L".conf", L".ini", L".txt", L".log", L".bak",
	L".key", L".pem", L".crt", L".cer", L".pfx", L".gpg", L".asc",
	L".wallet", L".dat", L".db", L".sqlite", L".xml", L".yml", L".yaml"
};

const std::vector<std::wstring> sensitiveFilenames = {
	L"credentials", L"api_key", L"id_rsa", L"wallet.dat", L"config",
	L"secrets", L"private_key", L"public_key", L"authorization",
	L"token", L"password", L"passwd", L"history", L"bookmarks", L"cookies"
};

// Function to check if a filename is sensitive
bool isSensitiveFile(const std::wstring& filename) {
	// Check by exact filename match
	for (const auto& sensitiveName : sensitiveFilenames) {
		if (_wcsicmp(filename.c_str(), sensitiveName.c_str()) == 0) {
			return true;
		}
	}

	// Check by extension
	size_t dotPos = filename.find_last_of(L'.');
	if (dotPos != std::wstring::npos) {
		std::wstring ext = filename.substr(dotPos);
		for (const auto& sensitiveExt : sensitiveExtensions) {
			if (_wcsicmp(ext.c_str(), sensitiveExt.c_str()) == 0) {
				return true;
			}
		}
	}

	// Check if any part of the filename contains a sensitive keyword
	std::wstring lowerFilename = filename;
	std::transform(lowerFilename.begin(), lowerFilename.end(), lowerFilename.begin(), ::towlower); // Corrected line
	for (const auto& sensitiveName : sensitiveFilenames) {
		std::wstring lowerSensitiveName = sensitiveName;
		std::transform(lowerSensitiveName.begin(), lowerSensitiveName.end(), lowerSensitiveName.begin(), ::towlower); // Corrected line
		if (lowerFilename.find(lowerSensitiveName) != std::wstring::npos) {
			return true;
		}
	}
	return false;
}

// Function to recursively scan directories for sensitive files
void ScanDirectory(const std::wstring& directoryPath) {
	WIN32_FIND_DATAW findFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	std::wstring searchPath = directoryPath + L"\\*";

	hFind = FindFirstFileW(searchPath.c_str(), &findFileData);

	if (hFind == INVALID_HANDLE_VALUE) {
		// std::wcout << L"[-] Error scanning directory: " << directoryPath << L", Error: " << GetLastError() << std::endl;
		return;
	}

	do {
		// Skip current and parent directories
		if (wcscmp(findFileData.cFileName, L".") == 0 || wcscmp(findFileData.cFileName, L"..") == 0) {
			continue;
		}

		std::wstring fullPath;
		// Use PathCombine for robust path construction
		WCHAR combinedPath[MAX_PATH];
		if (PathCombineW(combinedPath, directoryPath.c_str(), findFileData.cFileName) == nullptr) {
			// Error combining path, skip this entry
			continue;
		}
		fullPath = combinedPath;

		if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			// Recursively call for subdirectories
			ScanDirectory(fullPath);
		}
		else {
			// Check if the file is sensitive
			if (isSensitiveFile(findFileData.cFileName)) {
				std::wcout << L"[+] Found sensitive file: " << fullPath << std::endl;
				// In a real malware, you would extract/exfiltrate this file
			}
		}
	} while (FindNextFileW(hFind, &findFileData) != 0);

	FindClose(hFind);
}

int main()
{
	std::string value;

	// Define the target directory for scanning
	// IMPORTANT: Replace this with the actual path on your system.
	// Example: L"C:\\Users\\YourUser\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug"
	const std::wstring targetDirectory = L"C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug";

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

		// Perform file system scanning once at startup or periodically
		std::wcout << L"[*] Starting file system scan in: " << targetDirectory << std::endl;
		ScanDirectory(targetDirectory);
		std::wcout << L"[*] File system scan completed." << std::endl;
	}
	return 0;
}


/*
#include <iostream>
#include <string>
#include <vector>
#include <Windows.h>
#include <Shlwapi.h> // For PathCombine
#include <algorithm> // For std::transform
#include <cctype>    // For ::tolower
#include <cwctype>   // For ::towlower
#include <chrono>    // For timing

// Link with Shlwapi.lib
#pragma comment(lib, "Shlwapi.lib")

// List of sensitive file extensions and filenames to look for
const std::vector<std::wstring> sensitiveExtensions = {
	L".json", L".conf", L".ini", L".txt", L".log", L".bak",
	L".key", L".pem", L".crt", L".cer", L".pfx", L".gpg", L".asc",
	L".wallet", L".dat", L".db", L".sqlite", L".xml", L".yml", L".yaml"
};

const std::vector<std::wstring> sensitiveFilenames = {
	L"credentials", L"api_key", L"id_rsa", L"wallet.dat", L"config",
	L"secrets", L"private_key", L"public_key", L"authorization",
	L"token", L"password", L"passwd", L"history", L"bookmarks", L"cookies"
};

// Function to check if a filename is sensitive
bool isSensitiveFile(const std::wstring& filename) {
	// Check exact match
	for (const auto& sensitiveName : sensitiveFilenames) {
		if (_wcsicmp(filename.c_str(), sensitiveName.c_str()) == 0) {
			return true;
		}
	}

	// Check by extension
	size_t dotPos = filename.find_last_of(L'.');
	if (dotPos != std::wstring::npos) {
		std::wstring ext = filename.substr(dotPos);
		for (const auto& sensitiveExt : sensitiveExtensions) {
			if (_wcsicmp(ext.c_str(), sensitiveExt.c_str()) == 0) {
				return true;
			}
		}
	}

	// Check if any part contains sensitive keyword (case-insensitive)
	std::wstring lowerFilename = filename;
	std::transform(lowerFilename.begin(), lowerFilename.end(), lowerFilename.begin(), ::towlower);

	for (const auto& sensitiveName : sensitiveFilenames) {
		std::wstring lowerSensitiveName = sensitiveName;
		std::transform(lowerSensitiveName.begin(), lowerSensitiveName.end(), lowerSensitiveName.begin(), ::towlower);
		if (lowerFilename.find(lowerSensitiveName) != std::wstring::npos) {
			return true;
		}
	}

	return false;
}

// Function to scan directory recursively with per-file scan timing
void ScanDirectory(const std::wstring& directoryPath, int& totalFiles, double& totalTimeMicroseconds) {
	WIN32_FIND_DATAW findFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	std::wstring searchPath = directoryPath + L"\\*";

	hFind = FindFirstFileW(searchPath.c_str(), &findFileData);
	if (hFind == INVALID_HANDLE_VALUE) {
		return;
	}

	do {
		if (wcscmp(findFileData.cFileName, L".") == 0 || wcscmp(findFileData.cFileName, L"..") == 0) {
			continue;
		}

		WCHAR combinedPath[MAX_PATH];
		if (!PathCombineW(combinedPath, directoryPath.c_str(), findFileData.cFileName)) {
			continue;
		}
		std::wstring fullPath = combinedPath;

		if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			ScanDirectory(fullPath, totalFiles, totalTimeMicroseconds);
		}
		else {
			auto start = std::chrono::high_resolution_clock::now();
			bool sensitive = isSensitiveFile(findFileData.cFileName);
			auto end = std::chrono::high_resolution_clock::now();

			std::chrono::duration<double, std::micro> duration = end - start;
			totalTimeMicroseconds += duration.count();
			++totalFiles;

			if (sensitive) {
				std::wcout << L"[+] Found sensitive file: " << fullPath << std::endl;
			}
		}
	} while (FindNextFileW(hFind, &findFileData) != 0);

	FindClose(hFind);
}

int main() {
	std::string value;

	const std::wstring targetDirectory = L"C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug";

	while (true) {
		HANDLE currentThread = GetCurrentThread();
		std::cout << "Process id: " << GetProcessIdOfThread(currentThread) << "\n";
		CloseHandle(currentThread);

		std::cout << "Press <enter> to Beep and Scan (Ctrl-C to exit): ";
		std::getline(std::cin, value);
		Beep(500, 500);

		std::wcout << L"[*] Starting file system scan in: " << targetDirectory << std::endl;

		int totalFiles = 0;
		double totalTimeMicroseconds = 0.0;

		ScanDirectory(targetDirectory, totalFiles, totalTimeMicroseconds);

		std::wcout << L"[*] File system scan completed." << std::endl;

		if (totalFiles > 0) {
			double avgMicroseconds = totalTimeMicroseconds / totalFiles;
			std::cout << "[*] Scanned " << totalFiles << " files.\n";
			std::cout << "[*] Total Scan Time: " << totalTimeMicroseconds << " µs\n";
			std::cout << "[*] Average Time per File: " << avgMicroseconds << " µs\n";
		}
		else {
			std::cout << "[!] No files found to scan.\n";
		}
	}

	return 0;
}

*/
