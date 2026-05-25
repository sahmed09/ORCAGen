#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <string>
#include <Windows.h>
#include <gdiplus.h>
#include <thread>
#include <chrono>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

// Global flag to control screenshot thread
bool g_isRunning = true;

// Function to get CLSID for encoder (needed for saving images)
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
    UINT num = 0;
    UINT size = 0;

    Gdiplus::ImageCodecInfo* pImageCodecInfo = NULL;

    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0)
        return -1;

    pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
    if (pImageCodecInfo == NULL)
        return -1;

    Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);

    for (UINT j = 0; j < num; ++j)
    {
        if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
        {
            *pClsid = pImageCodecInfo[j].Clsid;
            free(pImageCodecInfo);
            return j;
        }
    }

    free(pImageCodecInfo);
    return -1;
}

// Function to capture screenshot using BitBlt
bool CaptureScreen(const std::wstring& filename)
{
    // Get the device context of the screen
    HDC hScreenDC = GetDC(NULL);
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);

    // Get screen dimensions
    int width = GetDeviceCaps(hScreenDC, HORZRES);
    int height = GetDeviceCaps(hScreenDC, VERTRES);

    // Create a bitmap to store the screenshot
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);

    // Copy screen content to the bitmap using BitBlt
    BitBlt(hMemoryDC, 0, 0, width, height, hScreenDC, 0, 0, SRCCOPY);

    // Create GDI+ Bitmap from HBITMAP
    Gdiplus::Bitmap bitmap(hBitmap, NULL);

    // Get PNG encoder CLSID
    CLSID pngClsid;
    if (GetEncoderClsid(L"image/png", &pngClsid) == -1)
    {
        std::cerr << "[-] Failed to get PNG encoder CLSID" << std::endl;
        SelectObject(hMemoryDC, hOldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hMemoryDC);
        ReleaseDC(NULL, hScreenDC);
        return false;
    }

    // Save the bitmap to file
    Gdiplus::Status status = bitmap.Save(filename.c_str(), &pngClsid, NULL);

    // Cleanup
    SelectObject(hMemoryDC, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);
    ReleaseDC(NULL, hScreenDC);

    if (status == Gdiplus::Ok)
    {
        std::wcout << L"[+] Screenshot saved: " << filename << std::endl;
        return true;
    }
    else
    {
        std::cerr << "[-] Failed to save screenshot. Status code: " << status << std::endl;
        return false;
    }
}

// Screenshot thread function - captures screen periodically
void ScreenCaptureThread(int intervalSeconds)
{
    int captureCount = 0;

    while (g_isRunning)
    {
        // Generate filename with timestamp
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);

        std::wstringstream filename;
        filename << L"screenshot_"
            << std::put_time(std::localtime(&time), L"%Y%m%d_%H%M%S")
            << L"_" << std::setw(3) << std::setfill(L'0') << captureCount
            << L".png";

        // --- timing start ---
        auto start = std::chrono::high_resolution_clock::now();

        // Capture the screen
        CaptureScreen(filename.str());
        captureCount++;

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::micro> duration = end - start;

        std::cout << "[Time] Screenshot took "
            << duration.count()
            << " microseconds.\n";
        // --- timing end ---

        // Wait for the specified interval
        for (int i = 0; i < intervalSeconds && g_isRunning; i++)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    std::cout << "[*] Screen capture thread terminated. Total captures: "
        << captureCount << std::endl;
}

int main()
{
    // Initialize GDI+
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    std::cout << "[*] Screen Logger PoC Started" << std::endl;
    std::cout << "[*] Screenshots will be captured every 10 seconds" << std::endl;
    std::cout << "[*] Screenshots saved in current directory" << std::endl;
    std::cout << std::endl;

    // Start screenshot capture thread (captures every 10 seconds)
    std::thread screenshotThread(ScreenCaptureThread, 10);

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

    // Cleanup
    g_isRunning = false;
    if (screenshotThread.joinable())
    {
        screenshotThread.join();
    }

    GdiplusShutdown(gdiplusToken);

    return 0;
}
