#include <iostream>
#include <string>
#include <vector>
#include <Windows.h>

std::vector<std::wstring> targetKeywords = {
	L"wallet.dat", L"api_key", L"credentials", L"config", L"browser", L"password"
};

std::vector<std::wstring> targetExtensions = {
	L".txt", L".json", L".conf", L".ini", L".dat"
};

// Function to determine if the file matches target keywords or extensions
bool matchTarget(const std::wstring& filename)
{
	// Check for target keywords
	for (const auto& keyword : targetKeywords)
	{
		if (filename.find(keyword) != std::wstring::npos)
			return true;
	}

	// Check for target extensions
	for (const auto& ext : targetExtensions)
	{
		if (filename.size() >= ext.size() &&
			filename.compare(filename.size() - ext.size(), ext.size(), ext) == 0)
			return true;
	}

	return false;
}

// Function to check if a file appears to be a decoy based on its name
/*bool isDecoy(const std::wstring& filename)
{
	return filename.find(L"fake") != std::wstring::npos ||
		filename.find(L"mock") != std::wstring::npos ||
		filename.find(L"dummy") != std::wstring::npos;
}*/

// Recursive directory scanner with evaluation logic
/*void scanDirectory(const std::wstring& directory)
{
	std::wstring searchPath = directory + L"\\*";
	WIN32_FIND_DATAW findData;
	HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		std::wcerr << L"[!] Failed to scan directory: " << directory << std::endl;
		return;
	}

	do
	{
		const std::wstring itemName = findData.cFileName;
		if (itemName == L"." || itemName == L"..") continue;

		std::wstring fullPath = directory + L"\\" + itemName;

		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			scanDirectory(fullPath);  // Recurse into subdirectory
		}
		else
		{
			std::wcout << L"[Scan] Found file: " << fullPath << std::endl;

			bool isTarget = matchTarget(itemName);
			bool decoyReturned = isDecoy(itemName);

			if (isTarget && decoyReturned)
				std::wcout << L"[RESULT] TRUE POSITIVE (decoy returned for target)\n";
			else if (!isTarget && decoyReturned)
				std::wcout << L"[RESULT] FALSE POSITIVE (decoy returned for non-target!)\n";
			else if (isTarget && !decoyReturned)
				std::wcout << L"[RESULT] FALSE NEGATIVE (real target file returned!)\n";
			else
				std::wcout << L"[RESULT] TRUE NEGATIVE (real benign file)\n";
		}

	} while (FindNextFileW(hFind, &findData));

	FindClose(hFind);
}*/

void scanDirectory(const std::wstring & directory)
{
	std::wstring searchPath = directory + L"\\*";
	WIN32_FIND_DATAW findData;
	HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		std::wcerr << L"[!] Failed to scan directory: " << directory << std::endl;
		return;
	}

	do
	{
		const std::wstring itemName = findData.cFileName;
		if (itemName == L"." || itemName == L"..") continue;

		std::wstring fullPath = directory + L"\\" + itemName;

		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			// Recurse into subdirectories
			scanDirectory(fullPath);
		}
		else
		{
			if (matchTarget(itemName))
			{
				std::wcout << L"[+] Found target file: " << fullPath << std::endl;
			}
		}

	} while (FindNextFileW(hFind, &findData));

	FindClose(hFind);
}

int main()
{
	std::string value;
	const std::wstring baseDir = L"C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug";

	while (true)
	{
		// Output the current process Id
		HANDLE currentThread = GetCurrentThread();
		std::cout << "Process id: " << GetProcessIdOfThread(currentThread) << "\n";
		CloseHandle(currentThread);



		std::cout << "Press <enter> to Beep (Ctrl-C to exit): ";
		std::getline(std::cin, value);
		Beep(500, 500);

		// Simulate file system scanning
		std::wcout << L"[*] Scanning directory for sensitive files...\n";
		scanDirectory(baseDir);
	}

	return 0;
}
