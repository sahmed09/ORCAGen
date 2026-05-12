#include <iostream>
#include <string>
#include <Windows.h>
#include <fstream>   // For file operations
#include <chrono>    // For current time
#include <iomanip>   // For std::put_time
#include <sstream>   // For std::stringstream
#include <ctime>     // For time_t and tm structures

// Function to log clipboard content
void LogClipboardContent()
{
	// Open the clipboard
	if (!OpenClipboard(NULL))
	{
		std::cerr << "Error: Could not open clipboard." << std::endl;
		return;
	}

	// Check if the clipboard contains text data (CF_TEXT)
	if (IsClipboardFormatAvailable(CF_TEXT))
	{
		HANDLE hClipboardData = GetClipboardData(CF_TEXT);
		if (hClipboardData != NULL)
		{
			char* lpClipboardText = static_cast<char*>(GlobalLock(hClipboardData));
			if (lpClipboardText != NULL)
			{
				std::string clipboardContent(lpClipboardText);

				// Get current time for timestamp
				auto now = std::chrono::system_clock::now();
				std::time_t in_time_t = std::chrono::system_clock::to_time_t(now);

				// Use localtime_s instead of localtime
				std::tm tm_buf; // Declare a tm struct to hold the result
				errno_t err = localtime_s(&tm_buf, &in_time_t); // Call localtime_s

				if (err == 0) // Check for success
				{
					// Create a filename with timestamp
					std::stringstream ss;
					ss << "clipboard_log_" << std::put_time(&tm_buf, "%Y%m%d_%H%M%S") << ".txt";
					std::string filename = ss.str();

					// Save to file
					std::ofstream outFile(filename, std::ios_base::app); // Open in append mode
					if (outFile.is_open())
					{
						outFile << "Timestamp: " << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S") << "\n";
						outFile << "Content:\n" << clipboardContent << "\n";
						outFile << "----------------------------------------\n";
						std::cout << "[+] Clipboard content logged to " << filename << std::endl;
						outFile.close();
					}
					else
					{
						std::cerr << "Error: Could not open file for logging." << std::endl;
					}
				}
				else
				{
					std::cerr << "Error: localtime_s failed with error code " << err << std::endl;
				}

				GlobalUnlock(hClipboardData);
			}
		}
	}
	else
	{
		// Optionally, you can add a message if no text data is available
		// std::cout << "No text data available in clipboard." << std::endl;
	}

	// Close the clipboard
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

		std::cout << "Press <enter> to Beep (Ctrl-C to exit): ";
		std::getline(std::cin, value);
		Beep(500, 500);

		// Call the clipboard logging function
		LogClipboardContent();
	}
	return 0;
}