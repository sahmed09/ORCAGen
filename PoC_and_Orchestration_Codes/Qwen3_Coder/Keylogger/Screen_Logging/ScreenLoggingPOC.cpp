#include <iostream>
#include <string>
#include <Windows.h>
#include <fstream>
#include <vector>
#include <chrono>

// Function to capture screenshot and save to file
bool CaptureScreen(const std::string& filename) {
    // Get screen dimensions
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Create device contexts
    HDC hScreenDC = GetDC(NULL);
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);

    // Create bitmap
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, screenWidth, screenHeight);
    HGDIOBJ hOldObj = SelectObject(hMemoryDC, hBitmap);

    // Copy screen to memory device context
    BitBlt(hMemoryDC, 0, 0, screenWidth, screenHeight, hScreenDC, 0, 0, SRCCOPY | CAPTUREBLT);

    // Get bitmap info
    BITMAPINFOHEADER bi = { 0 };
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = screenWidth;
    bi.biHeight = -screenHeight;  // Negative for top-down DIB
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;

    DWORD dwBmpSize = ((screenWidth * bi.biBitCount + 31) / 32) * 4 * screenHeight;

    // Allocate memory for bitmap data
    std::vector<BYTE> bmpData(dwBmpSize);

    // Get bitmap data
    GetDIBits(hScreenDC, hBitmap, 0, screenHeight, bmpData.data(),
        (BITMAPINFO*)&bi, DIB_RGB_COLORS);

    // Save to file
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        ReleaseDC(NULL, hScreenDC);
        DeleteDC(hMemoryDC);
        DeleteObject(hBitmap);
        return false;
    }

    // Write bitmap header (simplified)
    BITMAPFILEHEADER bf = { 0 };
    bf.bfType = 0x4D42; // "BM"
    bf.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwBmpSize;
    bf.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    file.write((char*)&bf, sizeof(BITMAPFILEHEADER));
    file.write((char*)&bi, sizeof(BITMAPINFOHEADER));
    file.write((char*)bmpData.data(), dwBmpSize);

    file.close();

    // Cleanup
    ReleaseDC(NULL, hScreenDC);
    DeleteDC(hMemoryDC);
    DeleteObject(hBitmap);

    return true;
}

// Function to generate timestamp for filename
std::string GenerateTimestamp() {
    SYSTEMTIME st;
    GetSystemTime(&st);
    char buffer[64];
    sprintf_s(buffer, "%04d-%02d-%02d_%02d-%02d-%02d",
        st.wYear, st.wMonth, st.wDay,
        st.wHour, st.wMinute, st.wSecond);
    return std::string(buffer);
}

int main()
{
    std::string value;
    // int screenshotCount = 0;

    while (true)
    {
        // Output the current process Id
        HANDLE currentThread = GetCurrentThread();
        std::cout << "Process id: ";
        std::cout << GetProcessIdOfThread(currentThread);
        std::cout << "\n";
        CloseHandle(currentThread);

        // Take screenshot every 5 iterations
        // if (screenshotCount % 5 == 0) {
            // std::string filename = "screenshot_" + GenerateTimestamp() + ".bmp";
            // if (CaptureScreen(filename)) {
                // std::cout << "[+] Screenshot saved: " << filename << "\n";
            // }
            // else {
                // std::cout << "[-] Failed to capture screenshot\n";
            // }
        // }

        std::cout << "Press <enter> to Beep (Ctrl-C to exit): ";
        std::getline(std::cin, value);
        Beep(500, 500);

        std::string filename = "screenshot_" + GenerateTimestamp() + ".bmp";

        // Measure time before and after screenshot
        auto start = std::chrono::high_resolution_clock::now();
        bool success = CaptureScreen(filename);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double, std::micro> duration = end - start;

        if (success) {
            std::cout << "[+] Screenshot saved: " << filename << "\n";
        }
        else {
            std::cout << "[-] Failed to capture screenshot\n";
        }

        std::cout << "[Time] Screenshot took " << duration.count() << " microseconds.\n";

        // screenshotCount++;
    }

    return 0;
}