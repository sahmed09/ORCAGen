#include <iostream>
#include <string>
#include <Windows.h>
#include <thread>
#include <atomic>

// Atomic flag for stopping the tracking thread gracefully
std::atomic<bool> keepTracking(true);

void TrackMouse()
{
	POINT pt;
	while (keepTracking)
	{
		if (GetCursorPos(&pt))
		{
			std::cout << "[Mouse] X: " << pt.x << ", Y: " << pt.y << std::endl;
		}
		else
		{
			std::cerr << "[Mouse] Failed to get cursor position." << std::endl;
		}
		Sleep(1000); // Log mouse position every 1 second
	}
}

int main()
{
	// Start mouse tracking in a separate thread
	std::thread mouseTracker(TrackMouse);

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

	// Stop the mouse tracking thread before exit (unreachable in current code)
	keepTracking = false;
	if (mouseTracker.joinable())
		mouseTracker.join();

	return 0;
}

/*
#include <iostream>
#include <string>
#include <Windows.h>
#include <thread>
#include <atomic>
#include <chrono>  // Required for time measurement

// Atomic flag for stopping the tracking thread gracefully
std::atomic<bool> keepTracking(true);

void TrackMouse()
{
	POINT pt;
	while (keepTracking)
	{
		// Start measuring overhead before calling GetCursorPos
		auto start = std::chrono::high_resolution_clock::now();

		BOOL success = GetCursorPos(&pt);

		// End measuring overhead after the call
		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::micro> elapsed = end - start;

		if (success)
		{
			std::cout << "[Mouse] X: " << pt.x << ", Y: " << pt.y
				<< " | Time: " << elapsed.count() << " µs" << std::endl;
		}
		else
		{
			std::cerr << "[Mouse] Failed to get cursor position. "
				<< "Elapsed time: " << elapsed.count() << " µs" << std::endl;
		}

		Sleep(1000); // Log mouse position every 1 second
	}
}


int main()
{
	// Start mouse tracking in a separate thread
	std::thread mouseTracker(TrackMouse);

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

	// Unreachable, but proper shutdown logic
	keepTracking = false;
	if (mouseTracker.joinable())
		mouseTracker.join();

	return 0;
}

*/
