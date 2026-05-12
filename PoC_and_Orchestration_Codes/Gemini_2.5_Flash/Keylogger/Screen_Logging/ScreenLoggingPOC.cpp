#include <iostream>
#include <string>
#include <Windows.h>
#include <gdiplus.h> // For saving image to file (optional, can use raw BMP for simplicity)
#pragma comment(lib, "gdiplus.lib") // Link with Gdiplus.lib

// Function to capture a screenshot and save it to a file
bool CaptureScreen(const std::string& filename)
{
	// Initialize GDI+ (required for image encoding/saving)
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	HDC hScreen = GetDC(NULL); // Get screen device context
	int x = GetSystemMetrics(SM_CXSCREEN);
	int y = GetSystemMetrics(SM_CYSCREEN);

	HDC hCompatibleDC = CreateCompatibleDC(hScreen);
	HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, x, y);
	HGDIOBJ hOldBitmap = SelectObject(hCompatibleDC, hBitmap);

	// Copy the screen content to the compatible DC
	BOOL bResult = BitBlt(hCompatibleDC, 0, 0, x, y, hScreen, 0, 0, SRCCOPY);

	// Save the bitmap to a file
	if (bResult)
	{
		Gdiplus::Bitmap* pBitmap = new Gdiplus::Bitmap(hBitmap, NULL);
		CLSID clsidEncoder;

		// Get the CLSID for PNG encoder (or BMP, JPEG etc.)
		// This function retrieves the CLSID of an image encoder.
		// We'll look for "image/png" to save as PNG.
		// You can change "image/png" to "image/bmp" or "image/jpeg" if preferred.
		UINT num = 0;  // number of image encoders
		UINT size = 0; // size of the image encoder array in bytes
		Gdiplus::GetImageEncodersSize(&num, &size);
		if (size == 0) return false;

		Gdiplus::ImageCodecInfo * pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
		if (pImageCodecInfo == NULL) return false;

		Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);

		for (UINT j = 0; j < num; ++j)
		{
			if (wcscmp(pImageCodecInfo[j].MimeType, L"image/png") == 0) // Or L"image/bmp", L"image/jpeg"
			{
				clsidEncoder = pImageCodecInfo[j].Clsid;
				free(pImageCodecInfo);
				pBitmap->Save(std::wstring(filename.begin(), filename.end()).c_str(), &clsidEncoder, NULL);
				delete pBitmap;
				SelectObject(hCompatibleDC, hOldBitmap);
				DeleteObject(hBitmap);
				DeleteDC(hCompatibleDC);
				ReleaseDC(NULL, hScreen);
				Gdiplus::GdiplusShutdown(gdiplusToken);
				return true;
			}
		}
		free(pImageCodecInfo);
		delete pBitmap;
	}

	// Clean up GDI objects
	SelectObject(hCompatibleDC, hOldBitmap);
	DeleteObject(hBitmap);
	DeleteDC(hCompatibleDC);
	ReleaseDC(NULL, hScreen);
	Gdiplus::GdiplusShutdown(gdiplusToken);
	return false;
}

int main()
{
	std::string value;
	int screenshotCounter = 0;

	// To ensure GDI+ is initialized for the screenshot function, it's good practice
	// to initialize it once at the start of the application if it's going to be called repeatedly.
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);


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

		// Capture a screenshot periodically
		std::string filename = "screenshot_" + std::to_string(screenshotCounter++) + ".png";
		if (CaptureScreen(filename))
		{
			std::cout << "Screenshot captured and saved as: " << filename << "\n";
		}
		else
		{
			std::cerr << "Failed to capture screenshot!\n";
		}
	}

	// Shutdown GDI+ when the application exits
	Gdiplus::GdiplusShutdown(gdiplusToken);
	return 0;
}

/*
#include <iostream>
#include <string>
#include <Windows.h>
#include <gdiplus.h> // For saving image to file (optional, can use raw BMP for simplicity)
#pragma comment(lib, "gdiplus.lib") // Link with Gdiplus.lib
#include <chrono>

// Function to capture a screenshot and save it to a file
bool CaptureScreen(const std::string& filename)
{
	// Initialize GDI+ (required for image encoding/saving)
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	HDC hScreen = GetDC(NULL); // Get screen device context
	int x = GetSystemMetrics(SM_CXSCREEN);
	int y = GetSystemMetrics(SM_CYSCREEN);

	HDC hCompatibleDC = CreateCompatibleDC(hScreen);
	HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, x, y);
	HGDIOBJ hOldBitmap = SelectObject(hCompatibleDC, hBitmap);

	// Copy the screen content to the compatible DC
	BOOL bResult = BitBlt(hCompatibleDC, 0, 0, x, y, hScreen, 0, 0, SRCCOPY);

	// Save the bitmap to a file
	if (bResult)
	{
		Gdiplus::Bitmap* pBitmap = new Gdiplus::Bitmap(hBitmap, NULL);
		CLSID clsidEncoder;

		// Get the CLSID for PNG encoder (or BMP, JPEG etc.)
		// This function retrieves the CLSID of an image encoder.
		// We'll look for "image/png" to save as PNG.
		// You can change "image/png" to "image/bmp" or "image/jpeg" if preferred.
		UINT num = 0;  // number of image encoders
		UINT size = 0; // size of the image encoder array in bytes
		Gdiplus::GetImageEncodersSize(&num, &size);
		if (size == 0) return false;

		Gdiplus::ImageCodecInfo * pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
		if (pImageCodecInfo == NULL) return false;

		Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);

		for (UINT j = 0; j < num; ++j)
		{
			if (wcscmp(pImageCodecInfo[j].MimeType, L"image/png") == 0) // Or L"image/bmp", L"image/jpeg"
			{
				clsidEncoder = pImageCodecInfo[j].Clsid;
				free(pImageCodecInfo);
				pBitmap->Save(std::wstring(filename.begin(), filename.end()).c_str(), &clsidEncoder, NULL);
				delete pBitmap;
				SelectObject(hCompatibleDC, hOldBitmap);
				DeleteObject(hBitmap);
				DeleteDC(hCompatibleDC);
				ReleaseDC(NULL, hScreen);
				Gdiplus::GdiplusShutdown(gdiplusToken);
				return true;
			}
		}
		free(pImageCodecInfo);
		delete pBitmap;
	}

	// Clean up GDI objects
	SelectObject(hCompatibleDC, hOldBitmap);
	DeleteObject(hBitmap);
	DeleteDC(hCompatibleDC);
	ReleaseDC(NULL, hScreen);
	Gdiplus::GdiplusShutdown(gdiplusToken);
	return false;
}

int main()
{
	std::string value;
	int screenshotCounter = 0;

	// Initialize GDI+ once at the start
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

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

		// Time the screenshot capture
		auto start = std::chrono::high_resolution_clock::now();

		std::string filename = "screenshot_" + std::to_string(screenshotCounter++) + ".png";
		if (CaptureScreen(filename))
		{
			std::cout << "Screenshot captured and saved as: " << filename << "\n";
		}
		else
		{
			std::cerr << "Failed to capture screenshot!\n";
		}

		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::micro> duration = end - start;
		std::cout << "[Time] Screenshot took " << duration.count() << " microseconds.\n";
	}

	// Shutdown GDI+
	Gdiplus::GdiplusShutdown(gdiplusToken);
	return 0;
}
*/
