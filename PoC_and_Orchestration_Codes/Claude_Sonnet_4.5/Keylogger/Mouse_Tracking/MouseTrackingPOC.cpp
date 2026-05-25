#include <iostream>
#include <string>
#include <Windows.h>
#include <thread>
#include <atomic>
#include <chrono>   // <-- Added for timing

// Global flag to control mouse tracking thread
std::atomic<bool> g_running(true);

// Mouse tracking function that runs in a separate thread
void MouseTrackingThread()
{
    POINT lastPos = { -1, -1 };
    POINT currentPos;

    std::cout << "[*] Mouse tracking thread started\n";

    while (g_running)
    {
        // --- timing start ---
        auto start = std::chrono::high_resolution_clock::now();

        // Get current cursor position
        if (GetCursorPos(&currentPos))
        {
            // Measure elapsed time immediately after GetCursorPos
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::micro> elapsed = end - start;

            // Only log if position has changed
            if (currentPos.x != lastPos.x || currentPos.y != lastPos.y)
            {
                std::cout << "[MOUSE] Position: ("
                    << currentPos.x << ", "
                    << currentPos.y << ")"
                    << " | Time: " << elapsed.count() << " µs\n";

                lastPos = currentPos;
            }
        }
        else
        {
            std::cerr << "[ERROR] GetCursorPos failed with error: "
                << GetLastError() << "\n";
        }

        // Sleep to avoid excessive CPU usage (poll every 100ms)
        Sleep(100);
    }

    std::cout << "[*] Mouse tracking thread terminated\n";
}

int main()
{
    std::string value;

    // Start mouse tracking in a separate thread
    std::thread mouseThread(MouseTrackingThread);

    std::cout << "[*] ClipboardLoggerPOC started\n";
    std::cout << "[*] Mouse tracking is active\n";
    std::cout << "========================================\n\n";

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

    // Cleanup (won't be reached with Ctrl-C, but good practice)
    g_running = false;
    if (mouseThread.joinable())
    {
        mouseThread.join();
    }

    return 0;
}
