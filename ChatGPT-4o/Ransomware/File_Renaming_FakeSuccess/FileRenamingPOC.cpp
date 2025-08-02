#include <iostream>
#include <string>
#include <Windows.h>

void RenameFilesInDirectory(const std::wstring& directoryPath, const std::wstring& extensionToAppend)
{
	WIN32_FIND_DATA findFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;

	std::wstring searchPath = directoryPath + L"\\*";

	hFind = FindFirstFile(searchPath.c_str(), &findFileData);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		std::wcerr << L"[-] Failed to open directory: " << directoryPath << L"\n";
		return;
	}

	do
	{
		const std::wstring fileName = findFileData.cFileName;

		// Skip directories (including . and ..)
		if ((findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
			fileName == L"." || fileName == L"..")
		{
			continue;
		}

		std::wstring fullOldPath = directoryPath + L"\\" + fileName;
		std::wstring fullNewPath = fullOldPath + extensionToAppend;

		if (MoveFile(fullOldPath.c_str(), fullNewPath.c_str()))
		{
			std::wcout << L"[+] Renamed: " << fileName << L" -> " << fileName + extensionToAppend << L"\n";
		}
		else
		{
			std::wcerr << L"[!] Failed to rename: " << fileName << L"\n";
		}

	} while (FindNextFile(hFind, &findFileData) != 0);

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

		std::cout << "Press <enter> to Beep and rename files (Ctrl-C to exit): ";
		std::getline(std::cin, value);

		Beep(500, 500);

		// Call the file renaming function
		std::wstring targetDirectory = L"C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug\\Files";
		std::wstring extension = L".locked"; // Could also be .encrypted, .REvil, etc.
		RenameFilesInDirectory(targetDirectory, extension);
	}

	return 0;
}


/*
#include <iostream>
#include <string>
#include <Windows.h>
#include <chrono>

void RenameFilesInDirectory(const std::wstring& directoryPath, const std::wstring& extensionToAppend)
{
	WIN32_FIND_DATA findFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;

	std::wstring searchPath = directoryPath + L"\\*";
	hFind = FindFirstFile(searchPath.c_str(), &findFileData);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		std::wcerr << L"[-] Failed to open directory: " << directoryPath << L"\n";
		return;
	}

	int attemptCount = 0;
	double totalMicroseconds = 0.0;

	do
	{
		const std::wstring fileName = findFileData.cFileName;

		// Skip directories
		if ((findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
			fileName == L"." || fileName == L"..")
		{
			continue;
		}

		std::wstring fullOldPath = directoryPath + L"\\" + fileName;
		std::wstring fullNewPath = fullOldPath + extensionToAppend;

		auto start = std::chrono::high_resolution_clock::now();

		BOOL result = MoveFile(fullOldPath.c_str(), fullNewPath.c_str());

		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::micro> duration = end - start;

		totalMicroseconds += duration.count();
		++attemptCount;

		if (result)
		{
			std::wcout << L"[+] Renamed: " << fileName << L" -> " << fileName + extensionToAppend
				<< L" (Time: " << duration.count() << L" µs)\n";
		}
		else
		{
			std::wcerr << L"[!] Failed to rename: " << fileName
				<< L" (FakeSuccess or real failure) - Time: " << duration.count() << L" µs\n";
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

		std::wstring targetDirectory = L"C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug\\Files";
		std::wstring extension = L".locked";

		std::wcout << L"[*] Renaming files in target directory...\n";
		RenameFilesInDirectory(targetDirectory, extension);
		std::wcout << std::endl;
	}

	return 0;
}
*/
