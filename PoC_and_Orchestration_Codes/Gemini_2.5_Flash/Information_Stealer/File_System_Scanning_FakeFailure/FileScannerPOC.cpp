#include <iostream>
#include <string>
#include <vector>
#include <Windows.h>
#include <ShlObj.h> // For SHGetFolderPath and CSIDL_PROFILE
#include <algorithm> // Required for std::transform
#include <cctype>    // Required for ::tolower (or <ctype.h> for C-style tolower)

// Define target file extensions and names
const std::vector<std::string> TARGET_EXTENSIONS = {
	".json", ".conf", ".ini", ".txt", ".log", ".dat", ".key", ".pem", ".p12", ".cer", ".crt", ".der",
	".wallet", ".kdbx", ".sqlite", ".db", ".xml", ".yml", ".yaml"
};

const std::vector<std::string> TARGET_FILENAMES = {
	"credentials", "password", "api_key", "config", "wallet", "seed", "private", "secret",
	"browser_profile", "login", "history", "cookies", "autofill", "form_history", "webdata"
};

// Function to check if a filename or extension matches our targets
bool IsSensitiveFile(const std::string& filename) {
	std::string lowerFilename = filename;
	// Ensure <algorithm> is included for std::transform
	// Ensure <cctype> is included for ::tolower
	std::transform(lowerFilename.begin(), lowerFilename.end(), lowerFilename.begin(),
		[](unsigned char c) { return static_cast<unsigned char>(std::tolower(c)); }); // Use std::tolower with static_cast for safety

// Check by full filename
	for (const auto& targetName : TARGET_FILENAMES) {
		if (lowerFilename.find(targetName) != std::string::npos) {
			return true;
		}
	}

	// Check by extension
	size_t dotPos = lowerFilename.rfind('.');
	if (dotPos != std::string::npos) {
		std::string extension = lowerFilename.substr(dotPos);
		for (const auto& targetExt : TARGET_EXTENSIONS) {
			if (extension == targetExt) {
				return true;
			}
		}
	}
	return false;
}

// Function to recursively scan a directory for sensitive files using Windows API
void ScanDirectoryForSensitiveFiles_Recursive(const std::wstring& currentPath) {
	WIN32_FIND_DATAW findFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	std::wstring searchPath = currentPath + L"\\*"; // Search for all files/directories

	hFind = FindFirstFileW(searchPath.c_str(), &findFileData);

	if (hFind == INVALID_HANDLE_VALUE) {
		// std::wcerr << L"Error opening directory: " << currentPath << L" (Error: " << GetLastError() << L")" << std::endl;
		return; // Directory might not exist or access denied
	}

	do {
		std::wstring filename_w = findFileData.cFileName;

		// Skip "." and ".." entries
		if (filename_w == L"." || filename_w == L"..") {
			continue;
		}

		// Construct full path
		std::wstring fullPath_w = currentPath + L"\\" + filename_w;

		if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			// It's a directory, recurse into it
			ScanDirectoryForSensitiveFiles_Recursive(fullPath_w);
		}
		else {
			// It's a file, check if it's sensitive
			// Convert wstring to string for IsSensitiveFile
			// This conversion is basic and might not handle all Unicode characters correctly,
			// but it's sufficient for typical English filenames and common sensitive keywords.
			std::string fullPath_s(fullPath_w.begin(), fullPath_w.end());

			if (IsSensitiveFile(fullPath_s)) {
				std::wcout << L"[+] Found potential sensitive file: " << fullPath_w << std::endl;
				// In a real malware, you'd exfiltrate or further process this file
			}
		}
	} while (FindNextFileW(hFind, &findFileData) != 0);

	FindClose(hFind);
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

		// Define the target directory for scanning
		// For the specified directory: C:\Users\User\Documents\MalwareAnalysis\MalwareEvaluation-main\ClipboardLogger\Debug
		// You might need to adjust this path based on your exact setup and where the PoC is run from.
		std::wstring targetScanPath = L"C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug";

		// Example of getting user's Documents folder programmatically (more stealthy)
		// wchar_t myDocumentsPath[MAX_PATH];
		// if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_MYDOCUMENTS, NULL, 0, myDocumentsPath))) {
		//    targetScanPath = myDocumentsPath;
		// } else {
		//    std::wcerr << L"Failed to get My Documents path, using hardcoded default." << std::endl;
		// }

		std::cout << "Press <enter> to Beep (Ctrl-C to exit): ";
		std::getline(std::cin, value);
		Beep(500, 500);

		std::wcout << L"\n[*] Starting file system scan in: " << targetScanPath << std::endl;
		ScanDirectoryForSensitiveFiles_Recursive(targetScanPath);
		std::wcout << L"[*] File system scan complete.\n" << std::endl;
	}
	return 0;
}




/*
#include <iostream>
#include <string>
#include <vector>
#include <Windows.h>
#include <ShlObj.h>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <iomanip> // For std::setprecision

const std::vector<std::string> TARGET_EXTENSIONS = {
	".json", ".conf", ".ini", ".txt", ".log", ".dat", ".key", ".pem", ".p12", ".cer", ".crt", ".der",
	".wallet", ".kdbx", ".sqlite", ".db", ".xml", ".yml", ".yaml"
};

const std::vector<std::string> TARGET_FILENAMES = {
	"credentials", "password", "api_key", "config", "wallet", "seed", "private", "secret",
	"browser_profile", "login", "history", "cookies", "autofill", "form_history", "webdata"
};

bool IsSensitiveFile(const std::string& filename) {
	std::string lowerFilename = filename;
	std::transform(lowerFilename.begin(), lowerFilename.end(), lowerFilename.begin(),
		[](unsigned char c) { return static_cast<unsigned char>(std::tolower(c)); });

	for (const auto& targetName : TARGET_FILENAMES) {
		if (lowerFilename.find(targetName) != std::string::npos) {
			return true;
		}
	}

	size_t dotPos = lowerFilename.rfind('.');
	if (dotPos != std::string::npos) {
		std::string extension = lowerFilename.substr(dotPos);
		for (const auto& targetExt : TARGET_EXTENSIONS) {
			if (extension == targetExt) {
				return true;
			}
		}
	}
	return false;
}

void ScanDirectoryForSensitiveFiles_Recursive(const std::wstring& currentPath) {
	WIN32_FIND_DATAW findFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	std::wstring searchPath = currentPath + L"\\*";

	hFind = FindFirstFileW(searchPath.c_str(), &findFileData);
	if (hFind == INVALID_HANDLE_VALUE) return;

	do {
		std::wstring filename_w = findFileData.cFileName;
		if (filename_w == L"." || filename_w == L"..") continue;

		std::wstring fullPath_w = currentPath + L"\\" + filename_w;

		if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			ScanDirectoryForSensitiveFiles_Recursive(fullPath_w);
		}
		else {
			std::string fullPath_s(fullPath_w.begin(), fullPath_w.end());
			if (IsSensitiveFile(fullPath_s)) {
				std::wcout << L"[+] Found potential sensitive file: " << fullPath_w << std::endl;
			}
		}
	} while (FindNextFileW(hFind, &findFileData) != 0);

	FindClose(hFind);
}

int main() {
	std::string value;

	std::wstring targetScanPath = L"C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug";

	std::vector<double> scanTimesMicroseconds;

	while (true) {
		HANDLE currentThread = GetCurrentThread();
		std::cout << "Process id: " << GetProcessIdOfThread(currentThread) << "\n";
		CloseHandle(currentThread);

		std::cout << "Press <enter> to Beep and Scan 10x (Ctrl-C to exit): ";
		std::getline(std::cin, value);
		Beep(500, 500);

		scanTimesMicroseconds.clear();

		std::wcout << L"[*] Performing 10 scans in: " << targetScanPath << std::endl;

		for (int i = 0; i < 10; ++i) {
			auto start = std::chrono::high_resolution_clock::now();

			ScanDirectoryForSensitiveFiles_Recursive(targetScanPath);

			auto end = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double, std::micro> duration = end - start;
			double timeTaken = duration.count();

			std::cout << "[*] Scan " << (i + 1) << " Time: " << std::fixed << std::setprecision(2)
				<< timeTaken << " µs" << std::endl;

			scanTimesMicroseconds.push_back(timeTaken);
		}

		// Compute total and average time
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