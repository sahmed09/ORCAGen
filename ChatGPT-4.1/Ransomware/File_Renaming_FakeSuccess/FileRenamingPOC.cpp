#include <iostream>
#include <string>
#include <Windows.h>
#include <vector>

// Utility: Get all files (non-recursive or recursive if needed)
void RenameFilesInDirectory(const std::wstring& directory, const std::wstring& ext, const std::wstring& id = L"") {
	WIN32_FIND_DATA findFileData;
	std::wstring searchPath = directory + L"\\*";
	HANDLE hFind = FindFirstFile(searchPath.c_str(), &findFileData);

	if (hFind == INVALID_HANDLE_VALUE) {
		std::wcout << L"Could not open directory: " << directory << std::endl;
		return;
	}

	do {
		// Skip . and ..
		if (wcscmp(findFileData.cFileName, L".") == 0 || wcscmp(findFileData.cFileName, L"..") == 0)
			continue;

		std::wstring filePath = directory + L"\\" + findFileData.cFileName;

		// Only files (not directories)
		if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			// Find the last '.' in the filename
			size_t lastDot = std::wstring(findFileData.cFileName).find_last_of(L'.');
			std::wstring newFileName;
			if (lastDot != std::wstring::npos) {
				// Replace extension
				newFileName = std::wstring(findFileData.cFileName).substr(0, lastDot) + ext + id;
			}
			else {
				// Append extension
				newFileName = std::wstring(findFileData.cFileName) + ext + id;
			}

			std::wstring newPath = directory + L"\\" + newFileName;

			// Rename the file using MoveFileEx
			if (MoveFileEx(filePath.c_str(), newPath.c_str(), MOVEFILE_REPLACE_EXISTING)) {
				std::wcout << L"Renamed: " << filePath << L" --> " << newPath << std::endl;
			}
			else {
				std::wcout << L"Failed to rename: " << filePath << L" (" << GetLastError() << L")" << std::endl;
			}
		}
	} while (FindNextFile(hFind, &findFileData));

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

		std::cout << "Press <enter> to Beep, type 'r' + <enter> to rename files, Ctrl-C to exit: ";
		std::getline(std::cin, value);

		if (value == "r" || value == "R") {
			// Example: ransomware extension and fake victim id (base64 or similar)
			std::wstring directory = L"C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug\\Files";
			std::wstring ransomwareExt = L".locked"; // could be .REvil, .encrypted, etc.
			std::wstring victimId = L".VXNlcklE"; // base64 of "UserID" as example
			RenameFilesInDirectory(directory, ransomwareExt, victimId);
		}
		else {
			Beep(500, 500);
		}
	}
	return 0;
}
