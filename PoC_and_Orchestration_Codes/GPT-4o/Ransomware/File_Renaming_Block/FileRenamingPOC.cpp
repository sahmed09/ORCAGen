#include <iostream>
#include <string>
#include <Windows.h>
#include <filesystem>

void RenameFilesInDirectory(const std::wstring& directory, const std::wstring& extension = L".locked")
{
	WIN32_FIND_DATA findFileData;
	std::wstring searchPath = directory + L"\\*";
	HANDLE hFind = FindFirstFile(searchPath.c_str(), &findFileData);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		std::wcerr << L"[!] Failed to open directory: " << directory << std::endl;
		return;
	}

	do
	{
		// Skip directories
		if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;

		std::wstring originalFile = directory + L"\\" + findFileData.cFileName;

		// Append the extension
		std::wstring newFileName = originalFile + extension;

		if (MoveFileEx(originalFile.c_str(), newFileName.c_str(), MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH))
		{
			std::wcout << L"[+] Renamed: " << originalFile << L" -> " << newFileName << std::endl;
		}
		else
		{
			std::wcerr << L"[-] Failed to rename: " << originalFile << std::endl;
		}

	} while (FindNextFile(hFind, &findFileData) != 0);

	FindClose(hFind);
}

int main()
{
	std::string value;
	std::wstring targetDirectory = L"C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug\\Files";

	while (true)
	{
		HANDLE currentThread = GetCurrentThread();
		std::cout << "Process id: ";
		std::cout << GetProcessIdOfThread(currentThread) << "\n";
		CloseHandle(currentThread);

		std::cout << "Press <enter> to Beep and rename files (Ctrl-C to exit): ";
		std::getline(std::cin, value);

		// Simulated malicious activity
		Beep(500, 500);
		RenameFilesInDirectory(targetDirectory, L".locked"); // could also use ".encrypted", ".REvil", etc.
	}

	return 0;
}



/*
#include <iostream>
#include <string>
#include <Windows.h>
#include <chrono>
#include <filesystem>

void RenameFilesInDirectory(const std::wstring& directory, const std::wstring& extension = L".locked")
{
	WIN32_FIND_DATA findFileData;
	std::wstring searchPath = directory + L"\\*";
	HANDLE hFind = FindFirstFile(searchPath.c_str(), &findFileData);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		std::wcerr << L"[!] Failed to open directory: " << directory << std::endl;
		return;
	}

	int attemptCount = 0;
	double totalMicroseconds = 0.0;

	do
	{
		// Skip directories
		if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;

		std::wstring originalFile = directory + L"\\" + findFileData.cFileName;
		std::wstring newFileName = originalFile + extension;

		auto start = std::chrono::high_resolution_clock::now();

		BOOL result = MoveFileEx(originalFile.c_str(), newFileName.c_str(), MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);

		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::micro> duration = end - start;

		totalMicroseconds += duration.count();
		++attemptCount;

		if (result)
		{
			std::wcout << L"[+] Renamed: " << originalFile << L" -> " << newFileName
				<< L" (Time: " << duration.count() << L" µs)" << std::endl;
		}
		else
		{
			std::wcerr << L"[-] Failed to rename: " << originalFile
				<< L" (Time: " << duration.count() << L" µs)" << std::endl;
		}

	} while (FindNextFile(hFind, &findFileData) != 0);

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
	std::wstring targetDirectory = L"C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug\\Files";

	while (true)
	{
		HANDLE currentThread = GetCurrentThread();
		std::cout << "Process id: ";
		std::cout << GetProcessIdOfThread(currentThread) << "\n";
		CloseHandle(currentThread);

		std::cout << "Press <enter> to Beep and rename files (Ctrl-C to exit): ";
		std::getline(std::cin, value);

		Beep(500, 500);
		std::wcout << L"[*] Renaming files in target directory...\n";
		RenameFilesInDirectory(targetDirectory, L".locked");
		std::wcout << std::endl;
	}

	return 0;
}
*/
