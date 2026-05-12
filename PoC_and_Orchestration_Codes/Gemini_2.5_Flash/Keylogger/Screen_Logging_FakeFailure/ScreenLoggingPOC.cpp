#include <iostream>
#include <string>
#include <Windows.h>
#include <gdiplus.h> // For saving image (requires Gdiplus.lib)
#include <vector>

#pragma comment(lib, "gdiplus.lib") // Link with Gdiplus.lib

// Function to get encoder CLSID for an image format
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
	UINT num = 0;           // Number of image encoders
	UINT size = 0;          // Size of the image encoder array in bytes

	Gdiplus::ImageCodecInfo* pImageCodecInfo = NULL;

	Gdiplus::GetImageEncodersSize(&num, &size);
	if (size == 0)
		return -1; // No encoders found

	pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
	if (pImageCodecInfo == NULL)
		return -1; // Memory allocation failed

	Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);

	for (UINT j = 0; j < num; ++j) {
		if (wcscmp(format, pImageCodecInfo[j].MimeType) == 0) {
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j; // Found encoder
		}
	}

	free(pImageCodecInfo);
	return -1; // Encoder not found
}

// Function to capture a screenshot and save it
void CaptureScreenAndSave(const std::wstring& filename) {
	ULONG_PTR gdiplusToken;
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	// Get screen dimensions
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);

	// Create a device context for the screen
	HDC hdcScreen = GetDC(NULL);
	HDC hdcMem = CreateCompatibleDC(hdcScreen);

	// Create a compatible bitmap
	HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, screenWidth, screenHeight);
	SelectObject(hdcMem, hBitmap);

	// Copy the screen content to the bitmap
	BitBlt(hdcMem, 0, 0, screenWidth, screenHeight, hdcScreen, 0, 0, SRCCOPY);

	// Create a GDI+ Bitmap from the HBITMAP
	Gdiplus::Bitmap* pBitmap = new Gdiplus::Bitmap(hBitmap, NULL);

	// Save the bitmap as a PNG file (you can change "image/png" to "image/jpeg", "image/bmp", etc.)
	CLSID pngClsid;
	if (GetEncoderClsid(L"image/png", &pngClsid) != -1) {
		pBitmap->Save(filename.c_str(), &pngClsid);
		std::wcout << L"Screenshot saved to: " << filename << std::endl;
	}
	else {
		std::wcerr << L"Failed to find PNG encoder." << std::endl;
	}

	// Clean up
	delete pBitmap;
	DeleteObject(hBitmap);
	DeleteDC(hdcMem);
	ReleaseDC(NULL, hdcScreen);

	Gdiplus::GdiplusShutdown(gdiplusToken);
}

int main()
{
	std::string value;
	int screenshotCount = 0; // To name screenshots uniquely

	// For demonstration, take a screenshot every 5 seconds
	// In a real scenario, you might want to run this in a separate thread
	// to avoid blocking the main loop's responsiveness.
	const DWORD SCREENSHOT_INTERVAL_MS = 5000; // 5 seconds

	DWORD lastScreenshotTime = GetTickCount();

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

		// Check if it's time to take a screenshot
		DWORD currentTime = GetTickCount();
		if (currentTime - lastScreenshotTime >= SCREENSHOT_INTERVAL_MS) {
			screenshotCount++;
			std::wstring filename = L"screenshot_" + std::to_wstring(screenshotCount) + L".png";
			CaptureScreenAndSave(filename);
			lastScreenshotTime = currentTime;
		}
	}
	return 0;
}




/*

*/