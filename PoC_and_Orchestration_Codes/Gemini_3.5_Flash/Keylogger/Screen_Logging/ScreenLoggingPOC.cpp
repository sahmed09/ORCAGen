#include <windows.h>
#include <iostream>
#include <string>
#include <chrono>

// Function to capture the screen and save it as a BMP file
bool CaptureScreen(const std::wstring& filename) {
    // 1. Get handles to the desktop window and its Device Context (DC)
    HWND hwndDesktop = GetDesktopWindow();
    HDC hdcDesktop = GetDC(hwndDesktop);

    // 2. Create a compatible memory DC to perform the drawing
    HDC hdcMem = CreateCompatibleDC(hdcDesktop);
    if (!hdcMem) {
        ReleaseDC(hwndDesktop, hdcDesktop);
        return false;
    }

    // 3. Determine screen dimensions
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    // 4. Create a compatible bitmap surface
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcDesktop, width, height);
    if (!hBitmap) {
        DeleteDC(hdcMem);
        ReleaseDC(hwndDesktop, hdcDesktop);
        return false;
    }

    // 5. Select the bitmap into the memory DC and copy the screen bits
    HGDIOBJ hOldObj = SelectObject(hdcMem, hBitmap);
    // BitBlt is the primary API responsible for transferring pixel data
    if (!BitBlt(hdcMem, 0, 0, width, height, hdcDesktop, 0, 0, SRCCOPY)) {
        SelectObject(hdcMem, hOldObj);
        DeleteObject(hBitmap);
        DeleteDC(hdcMem);
        ReleaseDC(hwndDesktop, hdcDesktop);
        return false;
    }

    // 6. Setup BMP file headers
    BITMAPFILEHEADER bmfHdr;
    BITMAPINFOHEADER bi;

    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = width;
    bi.biHeight = height;
    bi.biPlanes = 1;
    bi.biBitCount = 32; // Using 32-bit color depth
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    DWORD dwBmpSize = ((width * bi.biBitCount + 31) / 32) * 4 * height;

    bmfHdr.bfType = 0x4D42; // "BM" ASCII
    bmfHdr.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwBmpSize;
    bmfHdr.bfReserved1 = 0;
    bmfHdr.bfReserved2 = 0;
    bmfHdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);

    // 7. Write data to file
    HANDLE hFile = CreateFileW(filename.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        SelectObject(hdcMem, hOldObj);
        DeleteObject(hBitmap);
        DeleteDC(hdcMem);
        ReleaseDC(hwndDesktop, hdcDesktop);
        return false;
    }

    // Extract the raw bits into a buffer or write directly
    BYTE* lpBits = new BYTE[dwBmpSize];
    GetDIBits(hdcDesktop, hBitmap, 0, height, lpBits, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

    DWORD dwBytesWritten = 0;
    WriteFile(hFile, &bmfHdr, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
    WriteFile(hFile, &bi, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);
    WriteFile(hFile, lpBits, dwBmpSize, &dwBytesWritten, NULL);

    // Cleanup resources
    delete[] lpBits;
    CloseHandle(hFile);
    SelectObject(hdcMem, hOldObj);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(hwndDesktop, hdcDesktop);

    return true;
}

int main()
{
    std::string value;
    int captureCount = 0;

    while (true)
    {
        // Output the current process Id
        HANDLE currentThread = GetCurrentThread();
        std::cout << "Process id: ";
        std::cout << GetProcessIdOfThread(currentThread);
        std::cout << "\n";
        CloseHandle(currentThread);

        std::cout << "Press <enter> to Beep & Capture Screen (Ctrl-C to exit): ";
        std::getline(std::cin, value);

        // Execute the original monitored action
        Beep(500, 500);

        // Execute the new behavior pattern to test hooking scope
        std::wstring filename = L"screenshot_" + std::to_wstring(++captureCount) + L".bmp";
        std::wcout << L"[*] Executing screen capture to: " << filename << L"...\n";

        auto start = std::chrono::high_resolution_clock::now();

        if (CaptureScreen(filename)) {
            std::cout << "[+] Screen capture succeeded.\n";
        }
        else {
            std::cout << "[-] Screen capture failed.\n";
        }

        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double, std::micro> duration = end - start;
        std::cout << "[Time] Screenshot took " << duration.count() << " microseconds.\n";
    }

    return 0;
}

/*
#include <windows.h>
#include <iostream>
#include <string>

// Function to capture the screen and save it as a BMP file
bool CaptureScreen(const std::wstring& filename) {
    // 1. Get handles to the desktop window and its Device Context (DC)
    HWND hwndDesktop = GetDesktopWindow();
    HDC hdcDesktop = GetDC(hwndDesktop);

    // 2. Create a compatible memory DC to perform the drawing
    HDC hdcMem = CreateCompatibleDC(hdcDesktop);
    if (!hdcMem) {
        ReleaseDC(hwndDesktop, hdcDesktop);
        return false;
    }

    // 3. Determine screen dimensions
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    // 4. Create a compatible bitmap surface
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcDesktop, width, height);
    if (!hBitmap) {
        DeleteDC(hdcMem);
        ReleaseDC(hwndDesktop, hdcDesktop);
        return false;
    }

    // 5. Select the bitmap into the memory DC and copy the screen bits
    HGDIOBJ hOldObj = SelectObject(hdcMem, hBitmap);
    // BitBlt is the primary API responsible for transferring pixel data
    if (!BitBlt(hdcMem, 0, 0, width, height, hdcDesktop, 0, 0, SRCCOPY)) {
        SelectObject(hdcMem, hOldObj);
        DeleteObject(hBitmap);
        DeleteDC(hdcMem);
        ReleaseDC(hwndDesktop, hdcDesktop);
        return false;
    }

    // 6. Setup BMP file headers
    BITMAPFILEHEADER bmfHdr;
    BITMAPINFOHEADER bi;

    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = width;
    bi.biHeight = height;
    bi.biPlanes = 1;
    bi.biBitCount = 32; // Using 32-bit color depth
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    DWORD dwBmpSize = ((width * bi.biBitCount + 31) / 32) * 4 * height;

    bmfHdr.bfType = 0x4D42; // "BM" ASCII
    bmfHdr.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwBmpSize;
    bmfHdr.bfReserved1 = 0;
    bmfHdr.bfReserved2 = 0;
    bmfHdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);

    // 7. Write data to file
    HANDLE hFile = CreateFileW(filename.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        SelectObject(hdcMem, hOldObj);
        DeleteObject(hBitmap);
        DeleteDC(hdcMem);
        ReleaseDC(hwndDesktop, hdcDesktop);
        return false;
    }

    // Extract the raw bits into a buffer or write directly
    BYTE* lpBits = new BYTE[dwBmpSize];
    GetDIBits(hdcDesktop, hBitmap, 0, height, lpBits, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

    DWORD dwBytesWritten = 0;
    WriteFile(hFile, &bmfHdr, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
    WriteFile(hFile, &bi, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);
    WriteFile(hFile, lpBits, dwBmpSize, &dwBytesWritten, NULL);

    // Cleanup resources
    delete[] lpBits;
    CloseHandle(hFile);
    SelectObject(hdcMem, hOldObj);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(hwndDesktop, hdcDesktop);

    return true;
}

int main()
{
    std::string value;
    int captureCount = 0;

    while (true)
    {
        // Output the current process Id
        HANDLE currentThread = GetCurrentThread();
        std::cout << "Process id: ";
        std::cout << GetProcessIdOfThread(currentThread);
        std::cout << "\n";
        CloseHandle(currentThread);

        std::cout << "Press <enter> to Beep & Capture Screen (Ctrl-C to exit): ";
        std::getline(std::cin, value);

        // Execute the original monitored action
        Beep(500, 500);

        // Execute the new behavior pattern to test hooking scope
        std::wstring filename = L"screenshot_" + std::to_wstring(++captureCount) + L".bmp";
        std::wcout << L"[*] Executing screen capture to: " << filename << L"...\n";

        if (CaptureScreen(filename)) {
            std::cout << "[+] Screen capture succeeded.\n";
        }
        else {
            std::cout << "[-] Screen capture failed.\n";
        }
    }
    return 0;
}
*/