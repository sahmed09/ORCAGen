#include <iostream>
#include <string>
#include <Windows.h>
#include <thread>
#include <chrono>

// Function to track mouse movement
void TrackMouseMovement()
{
	POINT cursorPos;
	while (true)
	{
		auto start = std::chrono::high_resolution_clock::now();

		if (GetCursorPos(&cursorPos))
		{
			auto end = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double, std::micro> elapsed = end - start;
			std::cout << "[Mouse Tracker] X: " << cursorPos.x << " | Y: " << cursorPos.y
				<< " | Time: " << elapsed.count() << " µs" << std::endl;
		}
		else
		{
			std::cerr << "[Mouse Tracker] Failed to get cursor position." << std::endl;
		}

		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}
/*
void TrackMouseMovement()
{
	POINT cursorPos;
	while (true)
	{
		if (GetCursorPos(&cursorPos))
		{
			std::cout << "[Mouse Tracker] X: " << cursorPos.x << " | Y: " << cursorPos.y << std::endl;
		}
		else
		{
			std::cerr << "[Mouse Tracker] Failed to get cursor position." << std::endl;
		}

		std::this_thread::sleep_for(std::chrono::seconds(1)); // Log every second
	}
}
*/

int main()
{
	std::string value;

	// Launch mouse tracking in a separate thread
	std::thread mouseThread(TrackMouseMovement);
	mouseThread.detach(); // Run independently

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

	return 0;
}
