#include <iostream>
#include <string>
#include <Windows.h>
#include <sstream>
#include <fstream>

// Utility function to save HBITMAP as BMP file
bool SaveBitmapToFile(HBITMAP hBitmap, const std::wstring& filename)
{
	BITMAP bmp;
	PBITMAPINFO pbmi;
	WORD cClrBits;
	HANDLE hf;                  // file handle
	BITMAPFILEHEADER hdr;       // bitmap file-header
	PBITMAPINFOHEADER pbih;     // bitmap info-header
	LPBYTE lpBits;              // memory pointer
	DWORD dwTotal, cb, dwWritten;
	BOOL bSuccess = FALSE;

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
	else
		cClrBits = 32;

	pbmi = (PBITMAPINFO)LocalAlloc(LPTR,
		sizeof(BITMAPINFOHEADER) +
		sizeof(RGBQUAD) * (1 << cClrBits));

	pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pbmi->bmiHeader.biWidth = bmp.bmWidth;
	pbmi->bmiHeader.biHeight = bmp.bmHeight;
	pbmi->bmiHeader.biPlanes = bmp.bmPlanes;
	pbmi->bmiHeader.biBitCount = bmp.bmBitsPixel;
	if (cClrBits < 24)
		pbmi->bmiHeader.biClrUsed = (1 << cClrBits);

	pbmi->bmiHeader.biCompression = BI_RGB;
	pbmi->bmiHeader.biSizeImage = ((pbmi->bmiHeader.biWidth * cClrBits + 31) & ~31) / 8
		* pbmi->bmiHeader.biHeight;
	pbmi->bmiHeader.biClrImportant = 0;

	// Allocate memory for the bitmap bits
	lpBits = (LPBYTE)GlobalAlloc(GMEM_FIXED, pbmi->bmiHeader.biSizeImage);
	if (!lpBits)
		return false;

	// Retrieve the color table (RGBQUAD array) and the bits
	HDC hDC = GetDC(NULL);
	if (!GetDIBits(hDC, hBitmap, 0, (WORD)pbmi->bmiHeader.biHeight, lpBits, pbmi, DIB_RGB_COLORS)) {
		ReleaseDC(NULL, hDC);
		GlobalFree((HGLOBAL)lpBits);
		LocalFree(pbmi);
		return false;
	}
	ReleaseDC(NULL, hDC);

	// Create the .BMP file
	hf = CreateFileW(filename.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hf == INVALID_HANDLE_VALUE)
	{
		GlobalFree((HGLOBAL)lpBits);
		LocalFree(pbmi);
		return false;
	}

	// Initialize the fields in the file header
	hdr.bfType = 0x4d42; // 'BM'
	hdr.bfSize = (DWORD)(sizeof(BITMAPFILEHEADER) + pbmi->bmiHeader.biSize + pbmi->bmiHeader.biClrUsed * sizeof(RGBQUAD) + pbmi->bmiHeader.biSizeImage);
	hdr.bfReserved1 = 0;
	hdr.bfReserved2 = 0;
	hdr.bfOffBits = (DWORD) sizeof(BITMAPFILEHEADER) + pbmi->bmiHeader.biSize + pbmi->bmiHeader.biClrUsed * sizeof(RGBQUAD);

	// Write the BITMAPFILEHEADER to file
	WriteFile(hf, (LPVOID)& hdr, sizeof(BITMAPFILEHEADER), &dwWritten, NULL);
	// Write the BITMAPINFOHEADER and RGBQUAD array to file
	WriteFile(hf, (LPVOID)& pbmi->bmiHeader, sizeof(BITMAPINFOHEADER) + pbmi->bmiHeader.biClrUsed * sizeof(RGBQUAD), &dwWritten, NULL);
	// Write the array of color indices to file
	dwTotal = pbmi->bmiHeader.biSizeImage;
	WriteFile(hf, (LPSTR)lpBits, dwTotal, &dwWritten, NULL);

	// Clean up
	CloseHandle(hf);
	GlobalFree((HGLOBAL)lpBits);
	LocalFree(pbmi);

	return true;
}

// Captures the full desktop into a bitmap and saves it as BMP
bool CaptureDesktopScreenshot(const std::wstring & filename)
{
	int x1, y1, x2, y2, w, h;
	HWND hDesktopWnd = GetDesktopWindow();
	HDC hDesktopDC = GetDC(hDesktopWnd);
	HDC hCaptureDC = CreateCompatibleDC(hDesktopDC);

	x1 = GetSystemMetrics(SM_XVIRTUALSCREEN);
	y1 = GetSystemMetrics(SM_YVIRTUALSCREEN);
	x2 = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	y2 = GetSystemMetrics(SM_CYVIRTUALSCREEN);

	w = x2;
	h = y2;

	HBITMAP hCaptureBitmap = CreateCompatibleBitmap(hDesktopDC, w, h);
	SelectObject(hCaptureDC, hCaptureBitmap);

	BitBlt(hCaptureDC, 0, 0, w, h, hDesktopDC, x1, y1, SRCCOPY | CAPTUREBLT);

	bool saved = SaveBitmapToFile(hCaptureBitmap, filename);

	// Clean up
	DeleteObject(hCaptureBitmap);
	DeleteDC(hCaptureDC);
	ReleaseDC(hDesktopWnd, hDesktopDC);

	return saved;
}

int main()
{
	std::string value;
	int screenshot_index = 0;

	while (true)
	{
		// Output the current process Id
		HANDLE currentThread = GetCurrentThread();
		std::cout << "Process id: ";
		std::cout << GetProcessIdOfThread(currentThread);
		std::cout << "\n";
		CloseHandle(currentThread);

		std::cout << "Press <enter> to capture screenshot & Beep (Ctrl-C to exit): ";
		std::getline(std::cin, value);

		// Take screenshot and save as BMP
		std::wstringstream ss;
		ss << L"screenshot_" << screenshot_index++ << L".bmp";
		if (CaptureDesktopScreenshot(ss.str()))
			std::wcout << L"[+] Screenshot saved to " << ss.str() << std::endl;
		else
			std::wcout << L"[-] Screenshot failed!" << std::endl;

		Beep(500, 500);
	}
	return 0;
}

/*
#include <iostream>
#include <string>
#include <Windows.h>
#include <sstream>
#include <fstream>
#include <chrono>

// Utility function to save HBITMAP as BMP file
bool SaveBitmapToFile(HBITMAP hBitmap, const std::wstring& filename)
{
	BITMAP bmp;
	PBITMAPINFO pbmi;
	WORD cClrBits;
	HANDLE hf;                  // file handle
	BITMAPFILEHEADER hdr;       // bitmap file-header
	PBITMAPINFOHEADER pbih;     // bitmap info-header
	LPBYTE lpBits;              // memory pointer
	DWORD dwTotal, cb, dwWritten;
	BOOL bSuccess = FALSE;

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
	else
		cClrBits = 32;

	pbmi = (PBITMAPINFO)LocalAlloc(LPTR,
		sizeof(BITMAPINFOHEADER) +
		sizeof(RGBQUAD) * (1 << cClrBits));

	pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pbmi->bmiHeader.biWidth = bmp.bmWidth;
	pbmi->bmiHeader.biHeight = bmp.bmHeight;
	pbmi->bmiHeader.biPlanes = bmp.bmPlanes;
	pbmi->bmiHeader.biBitCount = bmp.bmBitsPixel;
	if (cClrBits < 24)
		pbmi->bmiHeader.biClrUsed = (1 << cClrBits);

	pbmi->bmiHeader.biCompression = BI_RGB;
	pbmi->bmiHeader.biSizeImage = ((pbmi->bmiHeader.biWidth * cClrBits + 31) & ~31) / 8
		* pbmi->bmiHeader.biHeight;
	pbmi->bmiHeader.biClrImportant = 0;

	// Allocate memory for the bitmap bits
	lpBits = (LPBYTE)GlobalAlloc(GMEM_FIXED, pbmi->bmiHeader.biSizeImage);
	if (!lpBits)
		return false;

	// Retrieve the color table (RGBQUAD array) and the bits
	HDC hDC = GetDC(NULL);
	if (!GetDIBits(hDC, hBitmap, 0, (WORD)pbmi->bmiHeader.biHeight, lpBits, pbmi, DIB_RGB_COLORS)) {
		ReleaseDC(NULL, hDC);
		GlobalFree((HGLOBAL)lpBits);
		LocalFree(pbmi);
		return false;
	}
	ReleaseDC(NULL, hDC);

	// Create the .BMP file
	hf = CreateFileW(filename.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hf == INVALID_HANDLE_VALUE)
	{
		GlobalFree((HGLOBAL)lpBits);
		LocalFree(pbmi);
		return false;
	}

	// Initialize the fields in the file header
	hdr.bfType = 0x4d42; // 'BM'
	hdr.bfSize = (DWORD)(sizeof(BITMAPFILEHEADER) + pbmi->bmiHeader.biSize + pbmi->bmiHeader.biClrUsed * sizeof(RGBQUAD) + pbmi->bmiHeader.biSizeImage);
	hdr.bfReserved1 = 0;
	hdr.bfReserved2 = 0;
	hdr.bfOffBits = (DWORD) sizeof(BITMAPFILEHEADER) + pbmi->bmiHeader.biSize + pbmi->bmiHeader.biClrUsed * sizeof(RGBQUAD);

	// Write the BITMAPFILEHEADER to file
	WriteFile(hf, (LPVOID)& hdr, sizeof(BITMAPFILEHEADER), &dwWritten, NULL);
	// Write the BITMAPINFOHEADER and RGBQUAD array to file
	WriteFile(hf, (LPVOID)& pbmi->bmiHeader, sizeof(BITMAPINFOHEADER) + pbmi->bmiHeader.biClrUsed * sizeof(RGBQUAD), &dwWritten, NULL);
	// Write the array of color indices to file
	dwTotal = pbmi->bmiHeader.biSizeImage;
	WriteFile(hf, (LPSTR)lpBits, dwTotal, &dwWritten, NULL);

	// Clean up
	CloseHandle(hf);
	GlobalFree((HGLOBAL)lpBits);
	LocalFree(pbmi);

	return true;
}

// Captures the full desktop into a bitmap and saves it as BMP
bool CaptureDesktopScreenshot(const std::wstring & filename)
{
	int x1, y1, x2, y2, w, h;
	HWND hDesktopWnd = GetDesktopWindow();
	HDC hDesktopDC = GetDC(hDesktopWnd);
	HDC hCaptureDC = CreateCompatibleDC(hDesktopDC);

	x1 = GetSystemMetrics(SM_XVIRTUALSCREEN);
	y1 = GetSystemMetrics(SM_YVIRTUALSCREEN);
	x2 = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	y2 = GetSystemMetrics(SM_CYVIRTUALSCREEN);

	w = x2;
	h = y2;

	HBITMAP hCaptureBitmap = CreateCompatibleBitmap(hDesktopDC, w, h);
	SelectObject(hCaptureDC, hCaptureBitmap);

	BitBlt(hCaptureDC, 0, 0, w, h, hDesktopDC, x1, y1, SRCCOPY | CAPTUREBLT);

	bool saved = SaveBitmapToFile(hCaptureBitmap, filename);

	// Clean up
	DeleteObject(hCaptureBitmap);
	DeleteDC(hCaptureDC);
	ReleaseDC(hDesktopWnd, hDesktopDC);

	return saved;
}

int main()
{
	std::string value;
	int screenshot_index = 0;

	while (true)
	{
		// Output the current process Id
		HANDLE currentThread = GetCurrentThread();
		std::cout << "Process id: ";
		std::cout << GetProcessIdOfThread(currentThread);
		std::cout << "\n";
		CloseHandle(currentThread);

		std::cout << "Press <enter> to capture screenshot & Beep (Ctrl-C to exit): ";
		std::getline(std::cin, value);

		// Start timing
		auto start = std::chrono::high_resolution_clock::now();

		// Take screenshot and save as BMP
		std::wstringstream ss;
		ss << L"screenshot_" << screenshot_index++ << L".bmp";
		bool success = CaptureDesktopScreenshot(ss.str());

		// End timing
		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::micro> duration = end - start;

		if (success)
			std::wcout << L"[+] Screenshot saved to " << ss.str() << std::endl;
		else
			std::wcout << L"[-] Screenshot failed!" << std::endl;

		std::cout << "[Time] Screenshot took " << duration.count() << " microseconds.\n";

		Beep(500, 500);
	}
	return 0;
}


*/
