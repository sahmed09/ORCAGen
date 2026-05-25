#include <iostream>
#include <string>
#include <Windows.h>
#include <chrono>

int main()
{
	std::string value;
	POINT cursorPosition;

	while (true)
	{
		// 1. Output the current process ID
		HANDLE currentThread = GetCurrentThread();
		std::cout << "Process id: ";
		std::cout << GetProcessIdOfThread(currentThread);
		std::cout << "\n";
		CloseHandle(currentThread);

		auto start = std::chrono::high_resolution_clock::now();

		// 2. Track mouse movement (Get current cursor coordinates)
		// This populates the POINT structure with x and y screen coordinates
		if (GetCursorPos(&cursorPosition))
		{
			std::cout << "[*] Mouse Position -> X: " << cursorPosition.x
				<< ", Y: " << cursorPosition.y << "\n";
		}
		else
		{
			std::cerr << "[-] Failed to get cursor position. Error: "
				<< GetLastError() << "\n";
		}

		auto end = std::chrono::high_resolution_clock::now();

		std::chrono::duration<double, std::micro> duration = end - start;
		std::cout << "[Time] Mouse tracking took " << duration.count() << " microseconds.\n";

		// 3. Keep the original interactive loop control
		std::cout << "Press <enter> to Beep (Ctrl-C to exit): ";
		std::getline(std::cin, value);
		Beep(500, 500);
		std::cout << "-------------------------------------------\n";
	}
	return 0;
}

/*
#include <iostream>
#include <string>
#include <Windows.h>

int main()
{
	std::string value;
	POINT cursorPosition;

	while (true)
	{
		// 1. Output the current process ID
		HANDLE currentThread = GetCurrentThread();
		std::cout << "Process id: ";
		std::cout << GetProcessIdOfThread(currentThread);
		std::cout << "\n";
		CloseHandle(currentThread);

		// 2. Track mouse movement (Get current cursor coordinates)
		// This populates the POINT structure with x and y screen coordinates
		if (GetCursorPos(&cursorPosition))
		{
			std::cout << "[*] Mouse Position -> X: " << cursorPosition.x
				<< ", Y: " << cursorPosition.y << "\n";
		}
		else
		{
			std::cerr << "[-] Failed to get cursor position. Error: "
				<< GetLastError() << "\n";
		}

		// 3. Keep the original interactive loop control
		std::cout << "Press <enter> to Beep (Ctrl-C to exit): ";
		std::getline(std::cin, value);
		Beep(500, 500);
		std::cout << "-------------------------------------------\n";
	}
	return 0;
}
*/