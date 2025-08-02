#include <iostream>
#include <string>
#include <fstream>
#include <Windows.h>
#include <chrono>

void LogClipboardContent()
{
	if (!OpenClipboard(nullptr)) {
		std::cerr << "[-] Failed to open clipboard." << std::endl;
		return;
	}

	HANDLE hData = GetClipboardData(CF_TEXT);
	if (hData == nullptr) {
		std::cerr << "[-] No text data found in clipboard." << std::endl;
		CloseClipboard();
		return;
	}

	char* pszText = static_cast<char*>(GlobalLock(hData));
	if (pszText != nullptr) {
		std::ofstream logFile("clipboard_log.txt", std::ios::app);
		if (logFile.is_open()) {
			logFile << "[+] Clipboard Data: " << pszText << "\n";
			logFile.close();
			std::cout << "[+] Clipboard content logged." << std::endl;
		}
		else {
			std::cerr << "[-] Failed to open log file." << std::endl;
		}
		GlobalUnlock(hData);
	}
	else {
		std::cerr << "[-] Failed to lock global clipboard memory." << std::endl;
	}

	CloseClipboard();
}

int main()
{
	std::string value;
	while (true)
	{
		// Output the current process ID
		HANDLE currentThread = GetCurrentThread();
		std::cout << "Process id: ";
		std::cout << GetProcessIdOfThread(currentThread);
		std::cout << "\n";
		CloseHandle(currentThread);

		std::cout << "Press <enter> to Beep and log clipboard (Ctrl-C to exit): ";
		std::getline(std::cin, value);
		Beep(500, 500);

		// Capture clipboard contents after beep
		auto start = std::chrono::high_resolution_clock::now();
		LogClipboardContent();
		auto end = std::chrono::high_resolution_clock::now();

		std::chrono::duration<double, std::micro> duration = end - start;
		std::cout << "[Time] Clipboard read took " << duration.count() << " microseconds.\n";
	}

	return 0;
}
