#include <iostream>
#include <string>
#include <Windows.h>
#include <vector> // Required for std::vector
#include <filesystem> // C++17 for easier path manipulation, or use WinAPI directly

// For simplicity, we'll define a macro for the target directory
// In a real scenario, this would be dynamically determined or configurable.
#define TARGET_DIR L"C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug\\Files"
#define ENCRYPTED_EXTENSION L".locked"

// Function to simulate file encryption and renaming
void EncryptAndRenameFiles()
{
	std::wcout << L"[*] Attempting to 'encrypt' and rename files in: " << TARGET_DIR << std::endl;

	WIN32_FIND_DATAW findFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	std::wstring searchPath = TARGET_DIR;
	searchPath += L"\\*"; // Search for all files and directories

	hFind = FindFirstFileW(searchPath.c_str(), &findFileData);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		std::wcerr << L"Error: Unable to open directory " << TARGET_DIR << L" or no files found. Error code: " << GetLastError() << std::endl;
		return;
	}

	do
	{
		// Skip current directory "." and parent directory ".."
		if (wcscmp(findFileData.cFileName, L".") == 0 || wcscmp(findFileData.cFileName, L"..") == 0)
		{
			continue;
		}

		// Only process files, not directories
		if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			std::wstring originalFilePath = TARGET_DIR;
			originalFilePath += L"\\";
			originalFilePath += findFileData.cFileName;

			// Construct the new file path with the appended extension
			std::wstring newFilePath = originalFilePath;
			newFilePath += ENCRYPTED_EXTENSION;

			std::wcout << L"Attempting to rename: " << originalFilePath << L" to " << newFilePath << std::endl;

			// Simulate encryption (no actual encryption is done here)
			// In a real malware, encryption logic would be applied before renaming.

			// Rename the file
			if (MoveFileExW(originalFilePath.c_str(), newFilePath.c_str(), MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING))
			{
				std::wcout << L"Successfully renamed: " << findFileData.cFileName << std::endl;
			}
			else
			{
				std::wcerr << L"Failed to rename " << findFileData.cFileName << L". Error code: " << GetLastError() << std::endl;
			}
		}
	} while (FindNextFileW(hFind, &findFileData) != 0);

	FindClose(hFind);
	std::wcout << L"[*] File renaming process completed." << std::endl;
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

		// Call the file renaming function at the start or at some trigger point
		EncryptAndRenameFiles();
	}
	return 0;
}




/*
#include <iostream>
#include <string>
#include <Windows.h>
#include <vector> // Required for std::vector
#include <filesystem> // C++17 for easier path manipulation, or use WinAPI directly
#include <chrono>

// For simplicity, we'll define a macro for the target directory
// In a real scenario, this would be dynamically determined or configurable.
#define TARGET_DIR L"C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug\\Files"
#define ENCRYPTED_EXTENSION L".locked"

// Function to simulate file encryption and renaming
void EncryptAndRenameFiles()
{
	std::wcout << L"[*] Attempting to 'encrypt' and rename files in: " << TARGET_DIR << std::endl;

	WIN32_FIND_DATAW findFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	std::wstring searchPath = TARGET_DIR;
	searchPath += L"\\*"; // Search for all files and directories

	hFind = FindFirstFileW(searchPath.c_str(), &findFileData);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		std::wcerr << L"Error: Unable to open directory " << TARGET_DIR << L" or no files found. Error code: " << GetLastError() << std::endl;
		return;
	}

	int attemptCount = 0;
	double totalMicroseconds = 0.0;

	do
	{
		// Skip current directory "." and parent directory ".."
		if (wcscmp(findFileData.cFileName, L".") == 0 || wcscmp(findFileData.cFileName, L"..") == 0)
		{
			continue;
		}

		// Only process files, not directories
		if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			std::wstring originalFilePath = TARGET_DIR;
			originalFilePath += L"\\";
			originalFilePath += findFileData.cFileName;

			std::wstring newFilePath = originalFilePath + ENCRYPTED_EXTENSION;

			auto start = std::chrono::high_resolution_clock::now();

			BOOL result = MoveFileExW(originalFilePath.c_str(), newFilePath.c_str(),
				MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING);

			auto end = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double, std::micro> duration = end - start;

			totalMicroseconds += duration.count();
			++attemptCount;

			if (result)
			{
				std::wcout << L"[+] Renamed: " << originalFilePath
					<< L" -> " << newFilePath
					<< L" (Time: " << duration.count() << L" µs)" << std::endl;
			}
			else
			{
				std::wcerr << L"[-] Failed to rename: " << originalFilePath
					<< L" (Time: " << duration.count() << L" µs)" << std::endl;
			}
		}
	} while (FindNextFileW(hFind, &findFileData) != 0);

	FindClose(hFind);

	if (attemptCount > 0)
	{
		double avgTime = totalMicroseconds / attemptCount;
		std::wcout << L"[*] Total files attempted: " << attemptCount << std::endl;
		std::wcout << L"[*] Total time spent: " << totalMicroseconds << L" µs" << std::endl;
		std::wcout << L"[*] Average time per attempt: " << avgTime << L" µs" << std::endl;
	}
	else
	{
		std::wcout << L"[!] No files found to rename." << std::endl;
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

		std::cout << "Press <enter> to Beep (Ctrl-C to exit): ";
		std::getline(std::cin, value);
		Beep(500, 500);

		// Call the file renaming function at the start or at some trigger point
		EncryptAndRenameFiles();
	}
	return 0;
}
*/