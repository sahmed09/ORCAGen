#include <iostream>
#include <string>
#include <Windows.h>
#include <sstream>
#include <fstream>

// Helper: Save HBITMAP as BMP file
bool SaveBitmapToFile(HBITMAP hBitmap, const std::string& filename)
{
	BITMAP bmp;
	PBITMAPINFO pbmi;
	WORD cClrBits;

	// Retrieve the bitmap's color format, width, and height
	if (!GetObject(hBitmap, sizeof(BITMAP), (LPSTR)& bmp))
		return false;

	cClrBits = (WORD)(bmp.bmPlanes * bmp.bmBitsPixel);
	if (cClrBits == 1)
		cClrBits = 1;
	else if (cClrBits <= 4)
		cClrBits = 4;
	else if (cClrBits <= 8)
		cClrBits = 8;
	else if (cClrBits <= 16)
		cClrBits = 16;
	else if (cClrBits <= 24)
		cClrBits = 24;
	else cClrBits = 32;

	// Allocate memory for BITMAPINFO structure
	pbmi = (PBITMAPINFO)LocalAlloc(LPTR,
		sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * (1 << cClrBits));

	pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pbmi->bmiHeader.biWidth = bmp.bmWidth;
	pbmi->bmiHeader.biHeight = bmp.bmHeight;
	pbmi->bmiHeader.biPlanes = bmp.bmPlanes;
	pbmi->bmiHeader.biBitCount = bmp.bmBitsPixel;
	pbmi->bmiHeader.biCompression = BI_RGB;
	pbmi->bmiHeader.biSizeImage = ((bmp.bmWidth * cClrBits + 31) & ~31) / 8 * bmp.bmHeight;
	pbmi->bmiHeader.biClrUsed = 0;
	pbmi->bmiHeader.biClrImportant = 0;

	// Create file
	HANDLE hf = CreateFileA(filename.c_str(), GENERIC_READ | GENERIC_WRITE, (DWORD)0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hf == INVALID_HANDLE_VALUE)
		return false;

	DWORD dwBmpSize = pbmi->bmiHeader.biSizeImage;
	BYTE * lpBits = (BYTE*)GlobalAlloc(GMEM_FIXED, dwBmpSize);

	// Get the bitmap bits
	HDC hdc = GetDC(NULL);
	if (!GetDIBits(hdc, hBitmap, 0, (WORD)bmp.bmHeight, lpBits, pbmi, DIB_RGB_COLORS)) {
		ReleaseDC(NULL, hdc);
		GlobalFree(lpBits);
		CloseHandle(hf);
		return false;
	}
	ReleaseDC(NULL, hdc);

	// Write BITMAPFILEHEADER
	BITMAPFILEHEADER hdr;
	hdr.bfType = 0x4d42; // "BM"
	hdr.bfSize = (DWORD)(sizeof(BITMAPFILEHEADER) + pbmi->bmiHeader.biSize + dwBmpSize);
	hdr.bfReserved1 = 0;
	hdr.bfReserved2 = 0;
	hdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + pbmi->bmiHeader.biSize;

	DWORD dwTmp;
	WriteFile(hf, (LPVOID)& hdr, sizeof(BITMAPFILEHEADER), &dwTmp, NULL);
	WriteFile(hf, (LPVOID)& pbmi->bmiHeader, sizeof(BITMAPINFOHEADER), &dwTmp, NULL);
	WriteFile(hf, (LPSTR)lpBits, dwBmpSize, &dwTmp, NULL);

	// Cleanup
	GlobalFree(lpBits);
	CloseHandle(hf);
	LocalFree(pbmi);
	return true;
}

// Capture the full screen and save as BMP
void CaptureScreen(int screenshot_index)
{
	int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
	int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
	int w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	int h = GetSystemMetrics(SM_CYVIRTUALSCREEN);

	HDC hScreenDC = GetDC(NULL);
	HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
	HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, w, h);
	HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);

	BitBlt(hMemoryDC, 0, 0, w, h, hScreenDC, x, y, SRCCOPY);

	std::ostringstream oss;
	oss << "screen_" << screenshot_index << ".bmp";
	if (SaveBitmapToFile(hBitmap, oss.str()))
		std::cout << "[+] Screenshot saved: " << oss.str() << "\n";
	else
		std::cout << "[!] Screenshot failed\n";

	SelectObject(hMemoryDC, hOldBitmap);
	DeleteObject(hBitmap);
	DeleteDC(hMemoryDC);
	ReleaseDC(NULL, hScreenDC);
}

int main()
{
	std::string value;
	int screenshot_index = 0;

	DWORD lastScreenshotTick = GetTickCount();
	// const DWORD screenshotIntervalMs = 5000; // 5 seconds
	const DWORD screenshotIntervalMs = 0;

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

		// Take screenshot every interval
		DWORD now = GetTickCount();
		if (now - lastScreenshotTick > screenshotIntervalMs) {
			CaptureScreen(screenshot_index++);
			lastScreenshotTick = now;
		}
	}
	return 0;
}



/*
#include <iostream>
#include <string>
#include <Windows.h>
#include <sstream>
#include <fstream>
#include <chrono>  // <-- For timing

// Helper: Save HBITMAP as BMP file
bool SaveBitmapToFile(HBITMAP hBitmap, const std::string& filename)
{
	BITMAP bmp;
	PBITMAPINFO pbmi;
	WORD cClrBits;

	// Retrieve the bitmap's color format, width, and height
	if (!GetObject(hBitmap, sizeof(BITMAP), (LPSTR)& bmp))
		return false;

	cClrBits = (WORD)(bmp.bmPlanes * bmp.bmBitsPixel);
	if (cClrBits == 1)
		cClrBits = 1;
	else if (cClrBits <= 4)
		cClrBits = 4;
	else if (cClrBits <= 8)
		cClrBits = 8;
	else if (cClrBits <= 16)
		cClrBits = 16;
	else if (cClrBits <= 24)
		cClrBits = 24;
	else cClrBits = 32;

	// Allocate memory for BITMAPINFO structure
	pbmi = (PBITMAPINFO)LocalAlloc(LPTR,
		sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * (1 << cClrBits));

	pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pbmi->bmiHeader.biWidth = bmp.bmWidth;
	pbmi->bmiHeader.biHeight = bmp.bmHeight;
	pbmi->bmiHeader.biPlanes = bmp.bmPlanes;
	pbmi->bmiHeader.biBitCount = bmp.bmBitsPixel;
	pbmi->bmiHeader.biCompression = BI_RGB;
	pbmi->bmiHeader.biSizeImage = ((bmp.bmWidth * cClrBits + 31) & ~31) / 8 * bmp.bmHeight;
	pbmi->bmiHeader.biClrUsed = 0;
	pbmi->bmiHeader.biClrImportant = 0;

	// Create file
	HANDLE hf = CreateFileA(filename.c_str(), GENERIC_READ | GENERIC_WRITE, (DWORD)0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hf == INVALID_HANDLE_VALUE)
		return false;

	DWORD dwBmpSize = pbmi->bmiHeader.biSizeImage;
	BYTE * lpBits = (BYTE*)GlobalAlloc(GMEM_FIXED, dwBmpSize);

	// Get the bitmap bits
	HDC hdc = GetDC(NULL);
	if (!GetDIBits(hdc, hBitmap, 0, (WORD)bmp.bmHeight, lpBits, pbmi, DIB_RGB_COLORS)) {
		ReleaseDC(NULL, hdc);
		GlobalFree(lpBits);
		CloseHandle(hf);
		return false;
	}
	ReleaseDC(NULL, hdc);

	// Write BITMAPFILEHEADER
	BITMAPFILEHEADER hdr;
	hdr.bfType = 0x4d42; // "BM"
	hdr.bfSize = (DWORD)(sizeof(BITMAPFILEHEADER) + pbmi->bmiHeader.biSize + dwBmpSize);
	hdr.bfReserved1 = 0;
	hdr.bfReserved2 = 0;
	hdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + pbmi->bmiHeader.biSize;

	DWORD dwTmp;
	WriteFile(hf, (LPVOID)& hdr, sizeof(BITMAPFILEHEADER), &dwTmp, NULL);
	WriteFile(hf, (LPVOID)& pbmi->bmiHeader, sizeof(BITMAPINFOHEADER), &dwTmp, NULL);
	WriteFile(hf, (LPSTR)lpBits, dwBmpSize, &dwTmp, NULL);

	// Cleanup
	GlobalFree(lpBits);
	CloseHandle(hf);
	LocalFree(pbmi);
	return true;
}

// Capture the full screen and save as BMP, with timing overhead
void CaptureScreen(int screenshot_index)
{
	int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
	int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
	int w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	int h = GetSystemMetrics(SM_CYVIRTUALSCREEN);

	// Start timing
	auto start = std::chrono::high_resolution_clock::now();

	HDC hScreenDC = GetDC(NULL);
	HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
	HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, w, h);
	HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);

	BitBlt(hMemoryDC, 0, 0, w, h, hScreenDC, x, y, SRCCOPY);

	std::ostringstream oss;
	oss << "screen_" << screenshot_index << ".bmp";
	if (SaveBitmapToFile(hBitmap, oss.str()))
		std::cout << "[+] Screenshot saved: " << oss.str() << "\n";
	else
		std::cout << "[!] Screenshot failed\n";

	SelectObject(hMemoryDC, hOldBitmap);
	DeleteObject(hBitmap);
	DeleteDC(hMemoryDC);
	ReleaseDC(NULL, hScreenDC);

	// End timing and print duration
	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::micro> duration = end - start;
	std::cout << "[Time] Screenshot took " << duration.count() << " microseconds.\n";
}

int main()
{
	std::string value;
	int screenshot_index = 0;

	DWORD lastScreenshotTick = GetTickCount();
	// const DWORD screenshotIntervalMs = 5000; // 5 seconds
	const DWORD screenshotIntervalMs = 0;

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

		// Take screenshot every interval
		DWORD now = GetTickCount();
		if (now - lastScreenshotTick > screenshotIntervalMs) {
			CaptureScreen(screenshot_index++);
			lastScreenshotTick = now;
		}
	}
	return 0;
}
*/
