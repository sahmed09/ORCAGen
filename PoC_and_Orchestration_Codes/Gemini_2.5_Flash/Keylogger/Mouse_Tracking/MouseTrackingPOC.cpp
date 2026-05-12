#include <iostream>
#include <string>
#include <Windows.h>

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

		// Mouse Tracking Enhancement: Get and display current mouse cursor position
		POINT mousePos;
		if (GetCursorPos(&mousePos))
		{
			std::cout << "Mouse Position: X=" << mousePos.x << ", Y=" << mousePos.y << "\n";
		}
		else
		{
			std::cerr << "Error getting mouse position.\n";
		}

		std::cout << "Press <enter> to Beep (Ctrl-C to exit): ";
		std::getline(std::cin, value);
		Beep(500, 500);
	}
	return 0;
}



/*
#include <iostream>
#include <string>
#include <Windows.h>
#include <chrono>

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

		// Mouse Tracking Enhancement with Overhead Calculation
		POINT mousePos;
		auto start = std::chrono::high_resolution_clock::now();

		if (GetCursorPos(&mousePos))
		{
			auto end = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double, std::micro> elapsed = end - start;

			std::cout << "Mouse Position: X=" << mousePos.x << ", Y=" << mousePos.y
				<< " | Time: " << elapsed.count() << " µs\n";
		}
		else
		{
			std::cerr << "Error getting mouse position.\n";
		}

		std::cout << "Press <enter> to Beep (Ctrl-C to exit): ";
		std::getline(std::cin, value);
		Beep(500, 500);
	}
	return 0;
}
*/
