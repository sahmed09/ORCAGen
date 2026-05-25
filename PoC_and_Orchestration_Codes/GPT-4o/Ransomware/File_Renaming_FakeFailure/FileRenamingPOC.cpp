#include <iostream>
#include <string>
#include <Windows.h>
#include <vector>

const std::wstring TARGET_DIR = L"C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug\\Files\\";
const std::wstring CUSTOM_EXTENSION = L".locked";

// Function to rename a file by appending a custom extension
void RenameFilesInDirectory(const std::wstring& directory, const std::wstring& newExtension)
{
	WIN32_FIND_DATA findData;
	std::wstring searchPath = directory + L"*";

	HANDLE hFind = FindFirstFile(searchPath.c_str(), &findData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		std::wcerr << L"[-] Failed to list directory: " << directory << std::endl;
		return;
	}

	do
	{
		// Skip "." and ".." directories
		if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			continue;

		std::wstring originalFileName = findData.cFileName;
		std::wstring fullOldPath = directory + originalFileName;

		// Skip already renamed files
		if (originalFileName.find(newExtension) != std::wstring::npos)
			continue;

		// Construct new name
		std::wstring fullNewPath = fullOldPath + newExtension;

		if (MoveFileEx(fullOldPath.c_str(), fullNewPath.c_str(), MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH))
		{
			std::wcout << L"[+] Renamed: " << originalFileName << L" --> " << originalFileName + newExtension << std::endl;
		}
		else
		{
			std::wcerr << L"[-] Failed to rename: " << originalFileName << std::endl;
		}

	} while (FindNextFile(hFind, &findData) != 0);

	FindClose(hFind);
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

		std::cout << "Press <enter> to Beep and rename files (Ctrl-C to exit): ";
		std::getline(std::cin, value);

		Beep(500, 500);

		std::wcout << L"[*] Renaming files in directory...\n";
		RenameFilesInDirectory(TARGET_DIR, CUSTOM_EXTENSION);
	}

	return 0;
}



/*
#include <iostream>
#include <string>
#include <Windows.h>
#include <chrono>

const std::wstring TARGET_DIR = L"C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug\\Files\\";
const std::wstring CUSTOM_EXTENSION = L".locked";

// Function to rename a file by appending a custom extension and calculate timing overhead
void RenameFilesInDirectory(const std::wstring& directory, const std::wstring& newExtension)
{
	WIN32_FIND_DATA findData;
	std::wstring searchPath = directory + L"*";

	HANDLE hFind = FindFirstFile(searchPath.c_str(), &findData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		std::wcerr << L"[-] Failed to list directory: " << directory << std::endl;
		return;
	}

	int attemptCount = 0;
	double totalMicroseconds = 0.0;

	do
	{
		// Skip "." and ".." directories
		if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			continue;

		std::wstring originalFileName = findData.cFileName;
		std::wstring fullOldPath = directory + originalFileName;

		// Skip already renamed files
		if (originalFileName.find(newExtension) != std::wstring::npos)
			continue;

		std::wstring fullNewPath = fullOldPath + newExtension;

		auto start = std::chrono::high_resolution_clock::now();

		BOOL result = MoveFileEx(fullOldPath.c_str(), fullNewPath.c_str(), MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);

		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::micro> duration = end - start;

		totalMicroseconds += duration.count();
		++attemptCount;

		if (result)
		{
			std::wcout << L"[+] Renamed: " << originalFileName << L" --> " << originalFileName + newExtension
				<< L" (Time: " << duration.count() << L" µs)\n";
		}
		else
		{
			std::wcerr << L"[!] Failed to rename: " << originalFileName
				<< L" (FakeSuccess or real failure) - Time: " << duration.count() << L" µs\n";
		}

	} while (FindNextFile(hFind, &findData) != 0);

	FindClose(hFind);

	if (attemptCount > 0)
	{
		double avgTime = totalMicroseconds / attemptCount;
		std::wcout << L"[*] Total files attempted: " << attemptCount << L"\n";
		std::wcout << L"[*] Total time spent: " << totalMicroseconds << L" µs\n";
		std::wcout << L"[*] Average time per attempt: " << avgTime << L" µs\n";
	}
	else
	{
		std::wcout << L"[!] No files found to rename.\n";
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

		std::cout << "Press <enter> to Beep and rename files (Ctrl-C to exit): ";
		std::getline(std::cin, value);

		Beep(500, 500);

		std::wcout << L"[*] Renaming files in directory...\n";
		RenameFilesInDirectory(TARGET_DIR, CUSTOM_EXTENSION);
		std::wcout << std::endl;
	}

	return 0;
}
*/
