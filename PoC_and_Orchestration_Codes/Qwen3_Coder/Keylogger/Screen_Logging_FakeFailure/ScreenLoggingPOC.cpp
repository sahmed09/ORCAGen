#include <iostream>
#include <string>
#include <Windows.h>
#include <fstream>
#include <chrono>

#pragma comment(lib, "Gdi32.lib")

int screenshotCounter = 0;

// Helper to save bitmap to file
bool SaveBitmapToFile(HBITMAP hBitmap, HDC hDC, LPCSTR filename)
{
	BITMAP bmp;
	GetObject(hBitmap, sizeof(BITMAP), &bmp);

	BITMAPFILEHEADER bmfHeader;
	BITMAPINFOHEADER bi;

	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = bmp.bmWidth;
	bi.biHeight = -bmp.bmHeight;  // Negative for top-down bitmap
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	DWORD dwBmpSize = ((bmp.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmp.bmHeight;

	HANDLE hDIB = GlobalAlloc(GHND, dwBmpSize);
	char* lpbitmap = (char*)GlobalLock(hDIB);

	GetDIBits(hDC, hBitmap, 0, (UINT)bmp.bmHeight, lpbitmap, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

	std::ofstream file(filename, std::ios::out | std::ios::binary);
	if (!file) return false;

	bmfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	bmfHeader.bfSize = dwBmpSize + bmfHeader.bfOffBits;
	bmfHeader.bfType = 0x4D42;

	file.write((char*)&bmfHeader, sizeof(BITMAPFILEHEADER));
	file.write((char*)&bi, sizeof(BITMAPINFOHEADER));
	file.write(lpbitmap, dwBmpSize);

	GlobalUnlock(hDIB);
	GlobalFree(hDIB);
	file.close();

	return true;
}

// Capture full screen and save it
void CaptureScreen()
{
	int screenX = GetSystemMetrics(SM_CXSCREEN);
	int screenY = GetSystemMetrics(SM_CYSCREEN);

	HDC hScreen = GetDC(NULL);
	HDC hDC = CreateCompatibleDC(hScreen);
	HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, screenX, screenY);
	SelectObject(hDC, hBitmap);

	if (BitBlt(hDC, 0, 0, screenX, screenY, hScreen, 0, 0, SRCCOPY)) {
		char filename[256];
		sprintf_s(filename, "screenshot_%03d.bmp", screenshotCounter++);
		SaveBitmapToFile(hBitmap, hDC, filename);
		std::cout << "[*] Screenshot saved: " << filename << std::endl;
	}

	DeleteObject(hBitmap);
	DeleteDC(hDC);
	ReleaseDC(NULL, hScreen);
}

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

		std::cout << "Press <enter> to Beep and capture screen (Ctrl-C to exit): ";
		std::getline(std::cin, value);

		Beep(500, 500);

		auto start = std::chrono::high_resolution_clock::now();
		CaptureScreen();
		auto end = std::chrono::high_resolution_clock::now();

		std::chrono::duration<double, std::micro> duration = end - start;
		std::cout << "[Time] Screenshot took " << duration.count() << " microseconds.\n";

	}

	return 0;
}
