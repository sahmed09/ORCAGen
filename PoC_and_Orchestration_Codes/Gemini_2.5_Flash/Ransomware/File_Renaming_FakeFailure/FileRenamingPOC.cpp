#include <iostream>
#include <string>
#include <Windows.h> // For Win32 API functions
#include <vector>    // For storing file paths
#include <Shlwapi.h> // For PathCombine (link with Shlwapi.lib)

// Define the directory where the PoC will look for files to rename.
// IMPORTANT: Make sure this directory exists and contains some dummy files for testing.
// For example, create "test1.txt", "document.docx", etc., inside this folder.
const std::wstring TARGET_DIRECTORY = L"C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug\\Files\\*";
const std::wstring NEW_EXTENSION = L".locked"; // The extension to append

// Function to rename files in the target directory
void RenameFilesInDirectory()
{
	WIN32_FIND_DATA findFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	std::wstring currentFilePath;
	std::wstring newFilePath;

	std::wcout << L"[*] Attempting to rename files in: " << TARGET_DIRECTORY.substr(0, TARGET_DIRECTORY.length() - 2) << L"\n";

	// Start finding files in the target directory
	hFind = FindFirstFile(TARGET_DIRECTORY.c_str(), &findFileData);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		std::wcout << L"[-] Error: Could not open directory " << TARGET_DIRECTORY.substr(0, TARGET_DIRECTORY.length() - 2) << L". GetLastError: " << GetLastError() << L"\n";
		return;
	}

	do
	{
		// Skip directories and the current/parent directory entries
		if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			// Construct the full path of the current file
			currentFilePath = TARGET_DIRECTORY.substr(0, TARGET_DIRECTORY.length() - 1); // Remove the "*"
			currentFilePath += findFileData.cFileName;

			// Construct the new file path with the appended extension
			newFilePath = currentFilePath + NEW_EXTENSION;

			std::wcout << L"[+] Renaming: \"" << currentFilePath << L"\" to \"" << newFilePath << L"\"\n";

			// Attempt to rename the file
			if (MoveFileEx(currentFilePath.c_str(), newFilePath.c_str(), MOVEFILE_REPLACE_EXISTING))
			{
				std::wcout << L"    [+] Successfully renamed.\n";
			}
			else
			{
				std::wcout << L"    [-] Failed to rename. GetLastError: " << GetLastError() << L"\n";
			}
		}
	} while (FindNextFile(hFind, &findFileData) != 0);

	FindClose(hFind); // Close the find handle
	std::wcout << L"[*] Finished file renaming attempt.\n";
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

		// Call the file renaming function once at startup or in the loop
		// For demonstration, we'll call it once when the program starts.
		// You could also call it inside the while loop if you want it to attempt
		// renaming repeatedly.
		RenameFilesInDirectory();
	}
	return 0;
}




/*
#include <iostream>
#include <string>
#include <Windows.h>
#include <vector>
#include <chrono>    // For timing overhead
#include <Shlwapi.h> // For PathCombine (link with Shlwapi.lib)

const std::wstring TARGET_DIRECTORY = L"C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug\\Files\\*";
const std::wstring NEW_EXTENSION = L".locked";

void RenameFilesInDirectory()
{
	WIN32_FIND_DATA findFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	std::wstring currentFilePath;
	std::wstring newFilePath;

	std::wcout << L"[*] Attempting to rename files in: " << TARGET_DIRECTORY.substr(0, TARGET_DIRECTORY.length() - 2) << L"\n";

	hFind = FindFirstFile(TARGET_DIRECTORY.c_str(), &findFileData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		std::wcout << L"[-] Error: Could not open directory " << TARGET_DIRECTORY.substr(0, TARGET_DIRECTORY.length() - 2) << L". GetLastError: " << GetLastError() << L"\n";
		return;
	}

	int attemptCount = 0;
	double totalMicroseconds = 0.0;

	do
	{
		if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			currentFilePath = TARGET_DIRECTORY.substr(0, TARGET_DIRECTORY.length() - 1); // Remove the '*'
			currentFilePath += findFileData.cFileName;

			// Skip already renamed files
			if (currentFilePath.find(NEW_EXTENSION) != std::wstring::npos)
				continue;

			newFilePath = currentFilePath + NEW_EXTENSION;

			auto start = std::chrono::high_resolution_clock::now();

			BOOL result = MoveFileEx(currentFilePath.c_str(), newFilePath.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);

			auto end = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double, std::micro> duration = end - start;

			totalMicroseconds += duration.count();
			++attemptCount;

			if (result)
			{
				std::wcout << L"[+] Renamed: " << currentFilePath << L" --> " << newFilePath << L" (Time: " << duration.count() << L" µs)\n";
			}
			else
			{
				std::wcout << L"[!] Failed to rename: " << currentFilePath << L" (Error: " << GetLastError() << L") - Time: " << duration.count() << L" µs\n";
			}
		}
	} while (FindNextFile(hFind, &findFileData) != 0);

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
		HANDLE currentThread = GetCurrentThread();
		std::cout << "Process id: " << GetProcessIdOfThread(currentThread) << "\n";
		CloseHandle(currentThread);

		std::cout << "Press <enter> to Beep and rename files (Ctrl-C to exit): ";
		std::getline(std::cin, value);
		Beep(500, 500);

		RenameFilesInDirectory();
		std::wcout << std::endl;
	}

	return 0;
}
*/