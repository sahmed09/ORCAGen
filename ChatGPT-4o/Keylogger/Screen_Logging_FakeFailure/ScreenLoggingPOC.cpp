#include <windows.h>
#include <iostream>
#include <string>
#include <fstream>
#include <ctime>
#include <sstream>

void CaptureScreen(const std::wstring& filename)
{
	// Get the desktop device context
	HDC hScreenDC = GetDC(NULL);
	HDC hMemoryDC = CreateCompatibleDC(hScreenDC);

	// Get screen dimensions
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);

	// Create a compatible bitmap from the screen DC
	HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, screenWidth, screenHeight);
	HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);

	// Bit block transfer into our compatible memory DC
	BitBlt(hMemoryDC, 0, 0, screenWidth, screenHeight, hScreenDC, 0, 0, SRCCOPY);

	// Create a BITMAPFILEHEADER and BITMAPINFOHEADER for saving as BMP
	BITMAP bmp;
	GetObject(hBitmap, sizeof(BITMAP), &bmp);

	BITMAPFILEHEADER bmfHeader = { 0 };
	BITMAPINFOHEADER bi = { 0 };

	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = bmp.bmWidth;
	bi.biHeight = -bmp.bmHeight;  // Negative to indicate top-down DIB
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;

	DWORD dwBmpSize = bmp.bmWidth * bmp.bmHeight * 4;

	// Allocate memory for image
	BYTE* lpbitmap = new BYTE[dwBmpSize];
	GetDIBits(hMemoryDC, hBitmap, 0, (UINT)bmp.bmHeight, lpbitmap, (BITMAPINFO*)& bi, DIB_RGB_COLORS);

	// Create BMP file
	std::ofstream file(filename, std::ios::out | std::ios::binary);
	if (file)
	{
		bmfHeader.bfType = 0x4D42;
		bmfHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwBmpSize;
		bmfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

		file.write((char*)& bmfHeader, sizeof(BITMAPFILEHEADER));
		file.write((char*)& bi, sizeof(BITMAPINFOHEADER));
		file.write((char*)lpbitmap, dwBmpSize);
		file.close();

		std::wcout << L"[+] Screenshot saved: " << filename << std::endl;
	}

	// Cleanup
	delete[] lpbitmap;
	SelectObject(hMemoryDC, hOldBitmap);
	DeleteObject(hBitmap);
	DeleteDC(hMemoryDC);
	ReleaseDC(NULL, hScreenDC);
}

std::wstring GetTimestampedFilename()
{
	std::wstringstream filename;
	std::time_t now = std::time(0);
	std::tm localTime;
	localtime_s(&localTime, &now);

	filename << L"screenshot_"
		<< (localTime.tm_year + 1900) << L"-"
		<< (localTime.tm_mon + 1) << L"-"
		<< localTime.tm_mday << L"_"
		<< localTime.tm_hour << L"-"
		<< localTime.tm_min << L"-"
		<< localTime.tm_sec << L".bmp";

	return filename.str();
}

int main()
{
	std::string value;
	int counter = 0;

	while (true)
	{
		// Output the current process Id
		HANDLE currentThread = GetCurrentThread();
		std::cout << "Process id: " << GetProcessIdOfThread(currentThread) << "\n";
		CloseHandle(currentThread);

		std::cout << "Press <enter> to Beep (Ctrl-C to exit): ";
		std::getline(std::cin, value);
		Beep(500, 500);

		// Capture a screenshot every 3 interactions
		if (++counter % 3 == 0)
		{
			std::wstring filename = GetTimestampedFilename();
			CaptureScreen(filename);
		}
	}
	return 0;
}


/*
#include <windows.h>
#include <iostream>
#include <string>
#include <fstream>
#include <ctime>
#include <sstream>
#include <chrono>  // <-- For timing

void CaptureScreen(const std::wstring& filename)
{
	HDC hScreenDC = GetDC(NULL);
	HDC hMemoryDC = CreateCompatibleDC(hScreenDC);

	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);

	HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, screenWidth, screenHeight);
	HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);

	BitBlt(hMemoryDC, 0, 0, screenWidth, screenHeight, hScreenDC, 0, 0, SRCCOPY);

	BITMAP bmp;
	GetObject(hBitmap, sizeof(BITMAP), &bmp);

	BITMAPFILEHEADER bmfHeader = { 0 };
	BITMAPINFOHEADER bi = { 0 };

	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = bmp.bmWidth;
	bi.biHeight = -bmp.bmHeight;
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;

	DWORD dwBmpSize = bmp.bmWidth * bmp.bmHeight * 4;

	BYTE* lpbitmap = new BYTE[dwBmpSize];
	GetDIBits(hMemoryDC, hBitmap, 0, (UINT)bmp.bmHeight, lpbitmap, (BITMAPINFO*)& bi, DIB_RGB_COLORS);

	std::ofstream file(filename, std::ios::out | std::ios::binary);
	if (file)
	{
		bmfHeader.bfType = 0x4D42;
		bmfHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwBmpSize;
		bmfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

		file.write((char*)& bmfHeader, sizeof(BITMAPFILEHEADER));
		file.write((char*)& bi, sizeof(BITMAPINFOHEADER));
		file.write((char*)lpbitmap, dwBmpSize);
		file.close();

		std::wcout << L"[+] Screenshot saved: " << filename << std::endl;
	}

	delete[] lpbitmap;
	SelectObject(hMemoryDC, hOldBitmap);
	DeleteObject(hBitmap);
	DeleteDC(hMemoryDC);
	ReleaseDC(NULL, hScreenDC);
}

std::wstring GetTimestampedFilename()
{
	std::wstringstream filename;
	std::time_t now = std::time(0);
	std::tm localTime;
	localtime_s(&localTime, &now);

	filename << L"screenshot_"
		<< (localTime.tm_year + 1900) << L"-"
		<< (localTime.tm_mon + 1) << L"-"
		<< localTime.tm_mday << L"_"
		<< localTime.tm_hour << L"-"
		<< localTime.tm_min << L"-"
		<< localTime.tm_sec << L".bmp";

	return filename.str();
}

int main()
{
	std::string value;
	int counter = 0;

	while (true)
	{
		HANDLE currentThread = GetCurrentThread();
		std::cout << "Process id: " << GetProcessIdOfThread(currentThread) << "\n";
		CloseHandle(currentThread);

		std::cout << "Press <enter> to Beep (Ctrl-C to exit): ";
		std::getline(std::cin, value);
		Beep(500, 500);

		if (++counter % 3 == 0)
		{
			std::wstring filename = GetTimestampedFilename();

			auto start = std::chrono::high_resolution_clock::now();
			CaptureScreen(filename);
			auto end = std::chrono::high_resolution_clock::now();

			std::chrono::duration<double, std::micro> duration = end - start;
			std::cout << "[Time] Screenshot took " << duration.count() << " microseconds.\n";
		}
	}

	return 0;
}
*/
