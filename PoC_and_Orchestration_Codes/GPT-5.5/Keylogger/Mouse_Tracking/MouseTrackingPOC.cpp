#include <iostream>
#include <string>
#include <Windows.h>
#include <chrono>

void TrackMouseMovement()
{
	POINT cursorPos;

	if (GetCursorPos(&cursorPos))
	{
		std::cout << "[Mouse Tracking] X: " << cursorPos.x
			<< ", Y: " << cursorPos.y << std::endl;
	}
	else
	{
		std::cout << "[Mouse Tracking] Failed to get cursor position." << std::endl;
	}
}

int main()
{
	std::string value;

	while (true)
	{
		HANDLE currentThread = GetCurrentThread();

		std::cout << "Process id: ";
		std::cout << GetProcessIdOfThread(currentThread);
		std::cout << "\n";

		CloseHandle(currentThread);

		auto start = std::chrono::high_resolution_clock::now();

		TrackMouseMovement();

		auto end = std::chrono::high_resolution_clock::now();

		std::chrono::duration<double, std::micro> duration = end - start;
		std::cout << "[Time] Mouse tracking took " << duration.count() << " microseconds.\n";

		std::cout << "Press <enter> to Beep (Ctrl-C to exit): ";
		std::getline(std::cin, value);

		Beep(500, 500);

		Sleep(1000);
	}

	return 0;
}

/*
#include <iostream>
#include <string>
#include <Windows.h>

void TrackMouseMovement()
{
	POINT cursorPos;

	if (GetCursorPos(&cursorPos))
	{
		std::cout << "[Mouse Tracking] X: " << cursorPos.x
			<< ", Y: " << cursorPos.y << std::endl;
	}
	else
	{
		std::cout << "[Mouse Tracking] Failed to get cursor position." << std::endl;
	}
}

int main()
{
	std::string value;

	while (true)
	{
		HANDLE currentThread = GetCurrentThread();

		std::cout << "Process id: ";
		std::cout << GetProcessIdOfThread(currentThread);
		std::cout << "\n";

		CloseHandle(currentThread);

		TrackMouseMovement();

		std::cout << "Press <enter> to Beep (Ctrl-C to exit): ";
		std::getline(std::cin, value);

		Beep(500, 500);

		Sleep(1000);
	}

	return 0;
}
*/