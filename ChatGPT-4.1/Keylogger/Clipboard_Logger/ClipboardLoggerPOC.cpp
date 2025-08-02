#include <iostream>
#include <string>
#include <fstream>
#include <Windows.h>

// Helper function to log clipboard text to a file
void LogClipboardText()
{
	if (!OpenClipboard(nullptr))
		return;

	HANDLE hData = GetClipboardData(CF_UNICODETEXT); // Unicode clipboard text
	if (hData != nullptr)
	{
		LPCWSTR pText = static_cast<LPCWSTR>(GlobalLock(hData));
		if (pText != nullptr)
		{
			// Convert wide string to UTF-8 for logging
			int len = WideCharToMultiByte(CP_UTF8, 0, pText, -1, nullptr, 0, nullptr, nullptr);
			std::string utf8Text(len, 0);
			WideCharToMultiByte(CP_UTF8, 0, pText, -1, &utf8Text[0], len, nullptr, nullptr);

			// Append clipboard content to a log file
			std::ofstream logfile("clipboard_log.txt", std::ios::app);
			logfile << "Clipboard data: " << utf8Text << std::endl;
			logfile.close();

			GlobalUnlock(hData);
		}
	}

	CloseClipboard();
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

		std::cout << "Press <enter> to log clipboard & Beep (Ctrl-C to exit): ";
		std::getline(std::cin, value);

		// Clipboard logging
		LogClipboardText();

		Beep(500, 500);
	}
	return 0;
}
