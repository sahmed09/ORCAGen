#include <iostream>
#include <string>
#include <vector>
#include <Windows.h>
#include <algorithm>
#include <chrono>

// --- File scanning config ---
const std::string kTargetDirectory = R"(C:\Users\User\Documents\MalwareAnalysis\MalwareEvaluation-main\ClipboardLogger\Debug)";
const std::vector<std::string> kTargetExtensions = {
	".json", ".conf", ".ini", ".txt", ".dat"
};
const std::vector<std::string> kTargetSubstrings = {
	"api_key", "wallet", "credential", "password", "secret", "config", "browser", "profile"
};

// Utility: convert to lowercase
std::string ToLower(const std::string& str) {
	std::string lower = str;
	std::transform(lower.begin(), lower.end(), lower.begin(),
		[](unsigned char c) { return std::tolower(c); });
	return lower;
}

// Checks if a filename matches any of the targets
bool IsSensitiveFile(const std::string& filename) {
	std::string lower = ToLower(filename);
	for (const auto& ext : kTargetExtensions) {
		if (lower.size() >= ext.size() &&
			lower.compare(lower.size() - ext.size(), ext.size(), ext) == 0)
			return true;
	}
	for (const auto& substr : kTargetSubstrings) {
		if (lower.find(substr) != std::string::npos)
			return true;
	}
	return false;
}

// Recursive directory scan using WinAPI
void ScanDirectory(const std::string & directory, std::vector<std::string> & results) {
	std::string searchPath = directory + "\\*";
	WIN32_FIND_DATAA findData;
	HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

	if (hFind == INVALID_HANDLE_VALUE)
		return;

	do {
		const char* name = findData.cFileName;

		// Skip "." and ".."
		if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
			continue;

		std::string fullPath = directory + "\\" + name;

		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			// Recurse into subdirectory
			ScanDirectory(fullPath, results);
		}
		else {
			if (IsSensitiveFile(name)) {
				results.push_back(fullPath);
			}
		}
	} while (FindNextFileA(hFind, &findData) != 0);

	FindClose(hFind);
}

// Entry point
int main() {
	std::string value;

	while (true) {
		// Output the current process Id
		HANDLE currentThread = GetCurrentThread();
		std::cout << "Process id: ";
		std::cout << GetProcessIdOfThread(currentThread);
		std::cout << "\n";
		CloseHandle(currentThread);

		std::cout << "Press <enter> to Beep (Ctrl-C to exit): ";
		std::getline(std::cin, value);
		Beep(500, 500);
		std::cout << std::endl;

		// --- File system scanning ---
		std::vector<std::string> sensitiveFiles;
		ScanDirectory(kTargetDirectory, sensitiveFiles);

		if (!sensitiveFiles.empty()) {
			std::cout << "[*] Sensitive files found:\n";
			for (const auto& file : sensitiveFiles) {
				std::cout << "    " << file << "\n";
			}
		}
		else {
			std::cout << "[*] No sensitive files found in: " << kTargetDirectory << "\n";
		}
		std::cout << std::endl;
	}
	return 0;
}

/*
#include <iostream>
#include <string>
#include <vector>
#include <Windows.h>
#include <algorithm>
#include <chrono>

// --- File scanning config ---
const std::string kTargetDirectory = R"(C:\Users\User\Documents\MalwareAnalysis\MalwareEvaluation-main\ClipboardLogger\Debug)";
const std::vector<std::string> kTargetExtensions = {
	".json", ".conf", ".ini", ".txt", ".dat"
};
const std::vector<std::string> kTargetSubstrings = {
	"api_key", "wallet", "credential", "password", "secret", "config", "browser", "profile"
};

// Utility: convert to lowercase
std::string ToLower(const std::string& str) {
	std::string lower = str;
	std::transform(lower.begin(), lower.end(), lower.begin(),
		[](unsigned char c) { return std::tolower(c); });
	return lower;
}

// Checks if a filename matches any of the targets
bool IsSensitiveFile(const std::string& filename) {
	std::string lower = ToLower(filename);
	for (const auto& ext : kTargetExtensions) {
		if (lower.size() >= ext.size() &&
			lower.compare(lower.size() - ext.size(), ext.size(), ext) == 0)
			return true;
	}
	for (const auto& substr : kTargetSubstrings) {
		if (lower.find(substr) != std::string::npos)
			return true;
	}
	return false;
}

// Recursive directory scan using WinAPI
// Updated ScanDirectory with timing logic
void ScanDirectory(const std::string& directory, std::vector<std::string>& results, int& totalFiles, double& totalTimeMicroseconds) {
	std::string searchPath = directory + "\\*";
	WIN32_FIND_DATAA findData;
	HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

	if (hFind == INVALID_HANDLE_VALUE)
		return;

	do {
		const char* name = findData.cFileName;

		if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
			continue;

		std::string fullPath = directory + "\\" + name;

		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			ScanDirectory(fullPath, results, totalFiles, totalTimeMicroseconds);
		}
		else {
			auto start = std::chrono::high_resolution_clock::now();

			bool isSensitive = IsSensitiveFile(name);

			auto end = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double, std::micro> duration = end - start;

			totalTimeMicroseconds += duration.count();
			++totalFiles;

			if (isSensitive) {
				results.push_back(fullPath);
			}
		}
	} while (FindNextFileA(hFind, &findData) != 0);

	FindClose(hFind);
}

// Entry point
int main() {
	std::string value;

	while (true) {
		HANDLE currentThread = GetCurrentThread();
		std::cout << "Process id: " << GetProcessIdOfThread(currentThread) << "\n";
		CloseHandle(currentThread);

		std::cout << "Press <enter> to Beep and Scan (Ctrl-C to exit): ";
		std::getline(std::cin, value);
		Beep(500, 500);
		std::cout << std::endl;

		std::vector<std::string> sensitiveFiles;
		int totalFiles = 0;
		double totalTimeMicroseconds = 0.0;

		ScanDirectory(kTargetDirectory, sensitiveFiles, totalFiles, totalTimeMicroseconds);

		if (!sensitiveFiles.empty()) {
			std::cout << "[*] Sensitive files found:\n";
			for (const auto& file : sensitiveFiles) {
				std::cout << "    " << file << "\n";
			}
		}
		else {
			std::cout << "[*] No sensitive files found in: " << kTargetDirectory << "\n";
		}

		if (totalFiles > 0) {
			double avgTime = totalTimeMicroseconds / totalFiles;
			std::cout << "[*] Scanned " << totalFiles << " files.\n";
			std::cout << "[*] Total Scan Time: " << totalTimeMicroseconds << " µs\n";
			std::cout << "[*] Average Time per File: " << avgTime << " µs\n";
		}
		else {
			std::cout << "[!] No files found to scan.\n";
		}

		std::cout << std::endl;
	}

	return 0;
}

*/
