#include <iostream>
#include <string>
#include <Windows.h>
#include <thread>
#include <atomic>
#include <chrono>

std::atomic<bool> gRunning(true);

bool SaveBitmapToFile(HBITMAP hBitmap, HDC hDC, const std::wstring& filename)
{
    BITMAP bmp;
    GetObject(hBitmap, sizeof(BITMAP), &bmp);

    BITMAPFILEHEADER bmfHeader;
    BITMAPINFOHEADER bi;

    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bmp.bmWidth;
    bi.biHeight = -bmp.bmHeight;   // top-down bitmap
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = bmp.bmWidth * bmp.bmHeight * 4;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    DWORD bmpSize = bi.biSizeImage;
    BYTE* bmpData = new BYTE[bmpSize];

    if (!GetDIBits(hDC, hBitmap, 0, bmp.bmHeight, bmpData,
        reinterpret_cast<BITMAPINFO*>(&bi), DIB_RGB_COLORS))
    {
        delete[] bmpData;
        return false;
    }

    HANDLE hFile = CreateFileW(
        filename.c_str(),
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (hFile == INVALID_HANDLE_VALUE)
    {
        delete[] bmpData;
        return false;
    }

    DWORD bytesWritten = 0;

    bmfHeader.bfType = 0x4D42; // "BM"
    bmfHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + bmpSize;
    bmfHeader.bfReserved1 = 0;
    bmfHeader.bfReserved2 = 0;
    bmfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    WriteFile(hFile, &bmfHeader, sizeof(BITMAPFILEHEADER), &bytesWritten, nullptr);
    WriteFile(hFile, &bi, sizeof(BITMAPINFOHEADER), &bytesWritten, nullptr);
    WriteFile(hFile, bmpData, bmpSize, &bytesWritten, nullptr);

    CloseHandle(hFile);
    delete[] bmpData;

    return true;
}

bool CaptureScreenshot(const std::wstring& filename)
{
    int screenX = GetSystemMetrics(SM_CXSCREEN);
    int screenY = GetSystemMetrics(SM_CYSCREEN);

    HDC hScreenDC = GetDC(nullptr);
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);

    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, screenX, screenY);
    HGDIOBJ oldBitmap = SelectObject(hMemoryDC, hBitmap);

    BOOL result = BitBlt(
        hMemoryDC,
        0,
        0,
        screenX,
        screenY,
        hScreenDC,
        0,
        0,
        SRCCOPY | CAPTUREBLT
    );

    bool saved = false;

    if (result)
    {
        saved = SaveBitmapToFile(hBitmap, hMemoryDC, filename);
    }

    SelectObject(hMemoryDC, oldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);
    ReleaseDC(nullptr, hScreenDC);

    return saved;
}

void ScreenLoggerThread()
{
    int count = 0;

    while (gRunning)
    {
        std::wstring filename = L"screenshot_" + std::to_wstring(count++) + L".bmp";

        auto start = std::chrono::high_resolution_clock::now();

        if (CaptureScreenshot(filename))
        {
            std::wcout << L"[+] Screenshot saved: " << filename << std::endl;
        }
        else
        {
            std::wcout << L"[-] Failed to capture screenshot." << std::endl;
        }

        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double, std::micro> duration = end - start;
        std::cout << "[Time] Screenshot took " << duration.count() << " microseconds.\n";

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

int main()
{
    std::string value;

    std::thread screenLogger(ScreenLoggerThread);

    while (true)
    {
        HANDLE currentThread = GetCurrentThread();

        std::cout << "Process id: ";
        std::cout << GetProcessIdOfThread(currentThread);
        std::cout << "\n";

        CloseHandle(currentThread);

        std::cout << "Press <enter> to Beep and continue screen logging (Ctrl-C to exit): ";
        std::getline(std::cin, value);

        Beep(500, 500);
    }

    gRunning = false;

    if (screenLogger.joinable())
        screenLogger.join();

    return 0;
}

/*
#include <iostream>
#include <string>
#include <Windows.h>
#include <thread>
#include <atomic>
#include <chrono>

std::atomic<bool> gRunning(true);

bool SaveBitmapToFile(HBITMAP hBitmap, HDC hDC, const std::wstring& filename)
{
    BITMAP bmp;
    GetObject(hBitmap, sizeof(BITMAP), &bmp);

    BITMAPFILEHEADER bmfHeader;
    BITMAPINFOHEADER bi;

    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bmp.bmWidth;
    bi.biHeight = -bmp.bmHeight;   // top-down bitmap
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = bmp.bmWidth * bmp.bmHeight * 4;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    DWORD bmpSize = bi.biSizeImage;
    BYTE* bmpData = new BYTE[bmpSize];

    if (!GetDIBits(hDC, hBitmap, 0, bmp.bmHeight, bmpData,
        reinterpret_cast<BITMAPINFO*>(&bi), DIB_RGB_COLORS))
    {
        delete[] bmpData;
        return false;
    }

    HANDLE hFile = CreateFileW(
        filename.c_str(),
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (hFile == INVALID_HANDLE_VALUE)
    {
        delete[] bmpData;
        return false;
    }

    DWORD bytesWritten = 0;

    bmfHeader.bfType = 0x4D42; // "BM"
    bmfHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + bmpSize;
    bmfHeader.bfReserved1 = 0;
    bmfHeader.bfReserved2 = 0;
    bmfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    WriteFile(hFile, &bmfHeader, sizeof(BITMAPFILEHEADER), &bytesWritten, nullptr);
    WriteFile(hFile, &bi, sizeof(BITMAPINFOHEADER), &bytesWritten, nullptr);
    WriteFile(hFile, bmpData, bmpSize, &bytesWritten, nullptr);

    CloseHandle(hFile);
    delete[] bmpData;

    return true;
}

bool CaptureScreenshot(const std::wstring& filename)
{
    int screenX = GetSystemMetrics(SM_CXSCREEN);
    int screenY = GetSystemMetrics(SM_CYSCREEN);

    HDC hScreenDC = GetDC(nullptr);
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);

    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, screenX, screenY);
    HGDIOBJ oldBitmap = SelectObject(hMemoryDC, hBitmap);

    BOOL result = BitBlt(
        hMemoryDC,
        0,
        0,
        screenX,
        screenY,
        hScreenDC,
        0,
        0,
        SRCCOPY | CAPTUREBLT
    );

    bool saved = false;

    if (result)
    {
        saved = SaveBitmapToFile(hBitmap, hMemoryDC, filename);
    }

    SelectObject(hMemoryDC, oldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);
    ReleaseDC(nullptr, hScreenDC);

    return saved;
}

void ScreenLoggerThread()
{
    int count = 0;

    while (gRunning)
    {
        std::wstring filename = L"screenshot_" + std::to_wstring(count++) + L".bmp";

        if (CaptureScreenshot(filename))
        {
            std::wcout << L"[+] Screenshot saved: " << filename << std::endl;
        }
        else
        {
            std::wcout << L"[-] Failed to capture screenshot." << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

int main()
{
    std::string value;

    std::thread screenLogger(ScreenLoggerThread);

    while (true)
    {
        HANDLE currentThread = GetCurrentThread();

        std::cout << "Process id: ";
        std::cout << GetProcessIdOfThread(currentThread);
        std::cout << "\n";

        CloseHandle(currentThread);

        std::cout << "Press <enter> to Beep and continue screen logging (Ctrl-C to exit): ";
        std::getline(std::cin, value);

        Beep(500, 500);
    }

    gRunning = false;

    if (screenLogger.joinable())
        screenLogger.join();

    return 0;
}
*/