#include <iostream>
#include <string>
#include <cstring>
#include <easyhook.h>

int main()
{
	DWORD processId;
	std::wcout << L"Enter the target process ID: ";
	std::cin >> processId;

	LPCWSTR dllToInject = L"..\\Debug\\AntiClipboardLoggerHook.dll";
	// WCHAR dllToInject[] = L"..\\Debug\\AntiFileRenamingHook.dll";
	std::wcout << L"Injecting DLL: " << dllToInject << std::endl;

	NTSTATUS nt = RhInjectLibrary(
		processId,                // Process ID
		0,                        // Thread ID (0 = all threads)
		EASYHOOK_INJECT_DEFAULT, // Flags
		(WCHAR*)dllToInject,             // 32-bit DLL
		NULL,                    // 64-bit DLL (if applicable)
		nullptr, 0               // No data to pass
	);

	if (nt != 0) {
		std::wcerr << L"[!] DLL injection failed. Error: " << nt << std::endl;
		std::wcerr << RtlGetLastErrorString() << std::endl;
	}
	else {
		std::wcout << L"[+] DLL successfully injected." << std::endl;
	}

	

	std::wcout << L"Press Enter to exit...";
	std::wstring dummy;
	std::getline(std::wcin, dummy);
	std::getline(std::wcin, dummy);

	return 0;
}
