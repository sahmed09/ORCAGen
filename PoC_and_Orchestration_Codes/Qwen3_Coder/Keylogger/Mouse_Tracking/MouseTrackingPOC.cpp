#include <iostream>
#include <string>
#include <Windows.h>
#include <thread>
#include <chrono>

// Mouse tracking function
void TrackMouseMovement() {
    POINT currentPos;
    static POINT lastPos = { 0, 0 };

    if (GetCursorPos(&currentPos)) {
        // Only log if mouse has moved significantly
        if (abs(currentPos.x - lastPos.x) > 5 || abs(currentPos.y - lastPos.y) > 5) {
            std::cout << "[MOUSE] Position: X=" << currentPos.x << ", Y=" << currentPos.y << std::endl;
            lastPos = currentPos;
        }
    }
}

int main()
{
    std::string value;
    DWORD lastBeepTime = 0;

    std::cout << "Mouse tracking PoC started. Press <enter> to Beep (Ctrl-C to exit):" << std::endl;

    while (true)
    {
        // Output the current process Id
        HANDLE currentThread = GetCurrentThread();
        std::cout << "Process id: ";
        std::cout << GetProcessIdOfThread(currentThread);
        std::cout << "\n";
        CloseHandle(currentThread);

        // Check for user input with timeout
        std::cout << "Press <enter> to Beep (Ctrl-C to exit): ";
        std::getline(std::cin, value);

        // Add some delay to prevent excessive CPU usage
        // std::this_thread::sleep_for(std::chrono::milliseconds(50));

        // Beep with different frequencies each time
        // DWORD currentTime = GetTickCount();
        // DWORD freq = 500 + (currentTime % 1000);
        // Beep(freq, 300);
        Beep(500, 500);

        // Log the beep event
        // std::cout << "[BEEP] Frequency: " << freq << "Hz" << std::endl;

        // Periodically track mouse movement (every 100ms)
        TrackMouseMovement();
    }

    return 0;
}



/*
#include <iostream>
#include <string>
#include <Windows.h>
#include <thread>
#include <chrono>

// Mouse tracking function
void TrackMouseMovement() {
    POINT currentPos;
    static POINT lastPos = { 0, 0 };

    auto start = std::chrono::high_resolution_clock::now(); // Start timing before the API call

    if (GetCursorPos(&currentPos)) {
        auto end = std::chrono::high_resolution_clock::now(); // End timing right after the call
        std::chrono::duration<double, std::micro> elapsed = end - start;

        // Only log if mouse has moved significantly
        if (abs(currentPos.x - lastPos.x) > 5 || abs(currentPos.y - lastPos.y) > 5) {
            std::cout << "[MOUSE] Position: X=" << currentPos.x
                << ", Y=" << currentPos.y
                << " | Time: " << elapsed.count() << " µs" << std::endl;

            lastPos = currentPos;
        }
        else {
            std::cout << "[MOUSE] Position unchanged | Time: " << elapsed.count() << " µs" << std::endl;
        }
    }
    else {
        auto end = std::chrono::high_resolution_clock::now(); // Still measure in failure path
        std::chrono::duration<double, std::micro> elapsed = end - start;
        std::cerr << "[Mouse Tracker] Failed to get cursor position. Time: "
            << elapsed.count() << " µs" << std::endl;
    }
}


int main()
{
    std::string value;
    DWORD lastBeepTime = 0;

    std::cout << "Mouse tracking PoC started. Press <enter> to Beep (Ctrl-C to exit):" << std::endl;

    while (true)
    {
        // Output the current process Id
        HANDLE currentThread = GetCurrentThread();
        std::cout << "Process id: ";
        std::cout << GetProcessIdOfThread(currentThread);
        std::cout << "\n";
        CloseHandle(currentThread);

        // Check for user input with timeout
        std::cout << "Press <enter> to Beep (Ctrl-C to exit): ";
        std::getline(std::cin, value);
        Beep(500, 500);

        TrackMouseMovement();
    }

    return 0;
}
*/