#include <iostream>
#include <string>
#include <Windows.h> // For Win32 APIs
#include <fstream>   // For std::ofstream to create dummy files
#include <vector>    // For std::vector (still useful for general C++ patterns, though not strictly required for this specific file operation)

// Function to rename files in a given directory with a custom extension
void RenameFilesInDirectory(const std::string& directoryPath, const std::string& newExtension)
{
	std::cout << "Attempting to rename files in: " << directoryPath << std::endl;

	WIN32_FIND_DATAA findFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	std::string searchPath = directoryPath + "\\*"; // Search all files in the directory

	// Start searching for files
	hFind = FindFirstFileA(searchPath.c_str(), &findFileData);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		std::cerr << "Error: No files found or directory inaccessible: " << directoryPath << ". Error code: " << GetLastError() << std::endl;
		return;
	}

	do
	{
		// Ignore "." and ".." directory entries
		if (strcmp(findFileData.cFileName, ".") != 0 && strcmp(findFileData.cFileName, "..") != 0)
		{
			// Check if it's a regular file (not a directory)
			if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				std::string originalFileName = findFileData.cFileName;
				std::string originalFilePath = directoryPath + "\\" + originalFileName;
				std::string newFilePath = originalFilePath + newExtension;

				// Convert std::string to LPCSTR (for MoveFileExA)
				LPCSTR lpExistingFileName = originalFilePath.c_str();
				LPCSTR lpNewFileName = newFilePath.c_str();

				if (MoveFileExA(lpExistingFileName, lpNewFileName, MOVEFILE_REPLACE_EXISTING))
				{
					std::cout << "Renamed: " << originalFilePath << " to " << newFilePath << std::endl;
				}
				else
				{
					DWORD error = GetLastError();
					std::cerr << "Failed to rename: " << originalFilePath << ". Error code: " << error << std::endl;
				}
			}
		}
	} while (FindNextFileA(hFind, &findFileData) != 0); // Continue to the next file

	FindClose(hFind); // Close the search handle
	std::cout << "File renaming process complete." << std::endl;
}

// Function to create a directory if it doesn't exist
BOOL CreateDirectoryIfNotExists(const std::string & path) {
	if (CreateDirectoryA(path.c_str(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS) {
		return TRUE;
	}
	return FALSE;
}

int main()
{
	// Define the target directory for file renaming.
	// IMPORTANT: Make sure this directory exists and contains some dummy files for testing.
	std::string targetDirectory = "C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug\\Files";
	std::string ransomwareExtension = ".locked";

	// Create the directory if it doesn't exist (for easier testing)
	if (!CreateDirectoryIfNotExists(targetDirectory)) {
		std::cerr << "Error creating directory: " << targetDirectory << ". Error code: " << GetLastError() << std::endl;
		// Optionally, handle this error more gracefully, perhaps exit or continue without file operations
	}
	else {
		std::cout << "Directory checked/created: " << targetDirectory << std::endl;
		// Create some dummy files for testing if the directory was just created or if you want to ensure they exist
		// This part could be more robust, e.g., only create if files don't exist
		for (int i = 0; i < 3; ++i) {
			std::string dummyFilePath = targetDirectory + "\\test_file_" + std::to_string(i) + ".txt";
			// Check if file already exists before creating
			HANDLE hFile = CreateFileA(dummyFilePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hFile != INVALID_HANDLE_VALUE) {
				CloseHandle(hFile); // File already exists
				// std::cout << "Dummy file already exists: " << dummyFilePath << std::endl;
			}
			else {
				// File does not exist, create it
				std::ofstream dummyFile(dummyFilePath);
				if (dummyFile.is_open()) {
					dummyFile << "This is a dummy file for testing file renaming." << std::endl;
					dummyFile.close();
					std::cout << "Created dummy file: " << dummyFilePath << std::endl;
				}
				else {
					std::cerr << "Error creating dummy file: " << dummyFilePath << std::endl;
				}
			}
		}
	}


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

		// Call the file renaming function after Beep
		RenameFilesInDirectory(targetDirectory, ransomwareExtension);
	}
	return 0;
}