# Clipboard Logging

## Initial Prompt

I’m currently developing a cyber deception framework that leverages API hooking. The project consists of three main components:

1. A proof-of-concept (PoC) malware sample that exhibits various behaviors.

2. A DLL injector capable of injecting arbitrary payloads into target processes.

3. An API hooking module, compiled as a DLL, which is injected into the malware process by the injector to monitor and intercept specific API calls.

Up next, I’ll provide you with the working code structure for each module so you can understand how to build fully functional implementations of all three components. In the next prompt, I’ll provide specific instructions. For now, please review the code to understand the project structure.

1. ClipboardLoggerPOC.cpp (a malware like application) and its code: 

```
#include <iostream>
#include <string>
#include <Windows.h>

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

		std::cout << "Press <enter> to Beep (Ctrl-C to exit): ";
		std::getline(std::cin, value);
		Beep(500, 500);
	}
	return 0;
}
```

2. InjectAntiClipboardHook.cpp: 

```
﻿#include <iostream>
#include <string>
#include <cstring>

#include <easyhook.h>

int main()
{
	DWORD processId;
	std::wcout << L"Enter the target process Id: ";
	std::cin >> processId;

	LPCWSTR dllToInject = L"..\\Debug\\AntiClipboardLoggerHook.dll";
	wprintf(L"Attempting to inject: %s\n\n", dllToInject);

	// Since we're not sending any data, pass nullptr and size 0
	NTSTATUS nt = RhInjectLibrary(
		processId,
		0,
		EASYHOOK_INJECT_DEFAULT,
		(WCHAR*)dllToInject,
		NULL,
		nullptr,
		0
	);

	if (nt != 0)
	{
		printf("RhInjectLibrary failed with error code = %d\n", nt);
		PWCHAR err = RtlGetLastErrorString();
		if (err != nullptr)
			std::wcout << err << "\n";
	}
	else
	{
		std::wcout << L"Library injected successfully.\n";
	}

	std::wcout << L"Press Enter to exit";
	std::wstring input;
	std::getline(std::wcin, input);
	std::getline(std::wcin, input);
	return 0;
}
```

injects the dll into the target application (malware in this case)

3. AntiClipboardLoggerHook (the dll component having Hooked functions)

```
#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <easyhook.h>

#pragma comment(lib, "EasyHook32.lib")  // or EasyHook64.lib depending on platform

using namespace std;

DWORD gFreqOffset = 0;

BOOL WINAPI myBeepHook(DWORD dwFreq, DWORD dwDuration)
{
	cout << "[+] BeepHook triggered!" << endl;
	cout << "Original Frequency: " << dwFreq << ", Duration: " << dwDuration << endl;
	return Beep(dwFreq + gFreqOffset, dwDuration);
}

// here goes to other hooked API functions

//...
//...

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO* inRemoteInfo)
{
	cout << "[*] Injection started." << endl;

	if (inRemoteInfo->UserDataSize == sizeof(DWORD))
	{
		gFreqOffset = *reinterpret_cast<DWORD*>(inRemoteInfo->UserData);
		cout << "Frequency Offset Received: " << gFreqOffset << endl;
	}

	HOOK_TRACE_INFO hHook = { NULL };
	FARPROC beepAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "Beep");
	cout << "Beep function address: " << (void*)beepAddr << endl;

	NTSTATUS result = LhInstallHook(beepAddr, myBeepHook, nullptr, &hHook);

	if (FAILED(result))
	{
		wstring s(RtlGetLastErrorString());
		wcerr << L"Failed to install hook: " << s << endl;
		return;
	}
	else
	{
		cout << "Beep hook installed successfully!" << endl;
	}

	ULONG ACLEntries[1] = { 0 };
	LhSetExclusiveACL(ACLEntries, 1, &hHook);
}
```

In the next prompt I will provide instructions what to do. For now analyze the code and understand the project structure.

## ChatGPT-4.1 Response

Thank you for providing the code and project structure! Here’s a clear breakdown and analysis based on what you’ve shared, so I’ll be fully ready for your specific instructions in the next step.

---

## 1. **ClipboardLoggerPOC.cpp** (PoC Malware Sample)

* **Purpose**: Simulates “malware-like” behavior for demo/testing.
* **Behavior**:

  * Continuously outputs the current process ID (by getting the current thread’s process ID).
  * Waits for user input and then triggers a Beep sound.
  * *Note*: This code is a safe placeholder; it doesn’t do actual clipboard logging yet, but sets up a “malicious” target for API interception/testing.

---

## 2. **InjectAntiClipboardHook.cpp** (DLL Injector)

* **Purpose**: Command-line injector to inject a DLL (payload) into a running process.
* **How it works**:

  * Prompts user for target process ID (the PoC malware or any other process).
  * Uses **EasyHook**’s `RhInjectLibrary` to inject `AntiClipboardLoggerHook.dll` into the process.
  * Handles and reports errors via EasyHook’s error API.
  * Waits for user input before closing, ensuring that the process stays alive for observation/debugging.
  * *Key detail*: The injector expects the DLL at a relative path (`..\\Debug\\AntiClipboardLoggerHook.dll`).

---

## 3. **AntiClipboardLoggerHook.dll** (API Hooking DLL)

* **Purpose**: This is the DLL injected into the target process; it’s responsible for hooking/intercepting Windows API functions.
* **Core functionality**:

  * **Global State**: `gFreqOffset` (demo, to alter Beep frequency).
  * **Hooked API Example**: Hooks the `Beep` function, logs the invocation, and can alter arguments (frequency).
  * **Injection Entry**: Implements `NativeInjectionEntryPoint` as the DLL’s injection entry, as required by EasyHook.
  * **Hook Installation**:

    * Locates the address of the target API (`Beep`).
    * Installs the hook using `LhInstallHook`.
    * Optionally sets up access control (exclusive ACL).
  * **Data Passing**: Can optionally receive parameters from the injector (`UserData`), e.g., to set offsets or configure deception dynamically.

---

## **High-level Project Flow**

1. **Compile** all modules: PoC “malware”, injector, and hooking DLL.
2. **Run** the PoC (ClipboardLoggerPOC.exe) to have a process for testing.
3. **Run** the injector, supplying the process ID of the PoC. This injects the DLL.
4. **The DLL** begins monitoring/intercepting API calls within the target process (demonstrated with `Beep`).

---

## **EasyHook Usage**

* Central to all DLL injection and API hooking.
* `RhInjectLibrary` for injection, `LhInstallHook` for function hooking.
* **Communication**: Can use `UserData` to pass parameters from injector to injected DLL at runtime.

---

## **Ready for Next Steps**

* The structure is modular and clean: *malware-like PoC* (target), *injector* (controller), and *hooking DLL* (payload/defender or attacker).
* The current example hooks `Beep`; other APIs (e.g., clipboard, keyboard, network) can be similarly hooked for deception or defense.
* With this in mind, I’m ready to help you expand, modify, or fully implement advanced orchestration, API coverage, decoy response logic, logging, or countermeasures—depending on your instructions!

---

**Please go ahead with your instructions whenever you’re ready!**


## PoC Code Generation Prompt

Enhance the malware proof-of-concept by adding a clipboard logging technique. This enhancement should enable the PoC to read data from the clipboard and save the captured content to a .txt file.

Output Language: C++

Technique: Clipboard Logging

Description: This technique monitors and extracts data from the system clipboard, such as copied text, passwords, or credit card numbers. It utilizes Windows APIs like GetClipboardData to access clipboard content.

A sample implementation is provided for reference. You’ll need to integrate the clipboard logging functionality into the given codebase.

ClipboardLoggerPOC.cpp (a malware like application) and its code: 

```
#include <iostream>
#include <string>
#include <Windows.h>

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

		std::cout << "Press <enter> to Beep (Ctrl-C to exit): ";
		std::getline(std::cin, value);
		Beep(500, 500);
	}
	return 0;
}
```

## ChatGPT-4.1 Response

Certainly! Here’s how you **enhance your PoC by adding clipboard logging functionality**.
The updated code will:

* Check the clipboard for text content (CF\_TEXT or CF\_UNICODETEXT).
* Save the clipboard data (if available) to a file named `clipboard_log.txt` in append mode every time the user presses <enter>.

---

### **Enhanced ClipboardLoggerPOC.cpp**

```cpp
#include <iostream>
#include <string>
#include <fstream>
#include <Windows.h>

// Helper function to log clipboard text to a file
void LogClipboardText()
{
    if (!OpenClipboard(nullptr))
        return;

    HANDLE hData = GetClipboardData(CF_UNICODETEXT); // Unicode clipboard text
    if (hData != nullptr)
    {
        LPCWSTR pText = static_cast<LPCWSTR>(GlobalLock(hData));
        if (pText != nullptr)
        {
            // Convert wide string to UTF-8 for logging
            int len = WideCharToMultiByte(CP_UTF8, 0, pText, -1, nullptr, 0, nullptr, nullptr);
            std::string utf8Text(len, 0);
            WideCharToMultiByte(CP_UTF8, 0, pText, -1, &utf8Text[0], len, nullptr, nullptr);

            // Append clipboard content to a log file
            std::ofstream logfile("clipboard_log.txt", std::ios::app);
            logfile << "Clipboard data: " << utf8Text << std::endl;
            logfile.close();

            GlobalUnlock(hData);
        }
    }

    CloseClipboard();
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

        std::cout << "Press <enter> to log clipboard & Beep (Ctrl-C to exit): ";
        std::getline(std::cin, value);

        // Clipboard logging
        LogClipboardText();

        Beep(500, 500);
    }
    return 0;
}
```

---

### **What was added?**

* A function `LogClipboardText()` that:

  * Opens the clipboard.
  * Attempts to read Unicode text.
  * Converts it to UTF-8.
  * Appends the data to `clipboard_log.txt` with a tag.
  * Closes the clipboard.
* This function is called before each beep when <enter> is pressed.

---

**Let me know if you need more advanced logging (e.g., filtering, time stamps, other clipboard formats), or integration with your defensive hook code!**


## Orchestration Code Generation Prompt

Develop an API hooking strategy using EasyHook to mislead the malware POC accessing clipboard content. Maintain normal clipboard functionality for legitimate applications. For your reference, the POC code of the malware, active defense strategy for API hooking and a sample API hooking code are given below.

Output Language: C++

Enhance the malware proof-of-concept by adding a clipboard logging technique. This enhancement should enable the PoC to read data from the clipboard and save the captured content to a .txt file.

POC code of the malware:

```
#include <iostream>
#include <string>
#include <fstream>
#include <Windows.h>

// Helper function to log clipboard text to a file
void LogClipboardText()
{
    if (!OpenClipboard(nullptr))
        return;

    HANDLE hData = GetClipboardData(CF_UNICODETEXT); // Unicode clipboard text
    if (hData != nullptr)
    {
        LPCWSTR pText = static_cast<LPCWSTR>(GlobalLock(hData));
        if (pText != nullptr)
        {
            // Convert wide string to UTF-8 for logging
            int len = WideCharToMultiByte(CP_UTF8, 0, pText, -1, nullptr, 0, nullptr, nullptr);
            std::string utf8Text(len, 0);
            WideCharToMultiByte(CP_UTF8, 0, pText, -1, &utf8Text[0], len, nullptr, nullptr);

            // Append clipboard content to a log file
            std::ofstream logfile("clipboard_log.txt", std::ios::app);
            logfile << "Clipboard data: " << utf8Text << std::endl;
            logfile.close();

            GlobalUnlock(hData);
        }
    }

    CloseClipboard();
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

        std::cout << "Press <enter> to log clipboard & Beep (Ctrl-C to exit): ";
        std::getline(std::cin, value);

        // Clipboard logging
        LogClipboardText();

        Beep(500, 500);
    }
    return 0;
}
```

Active Defense: Intercept clipboard read operations and replace actual data with decoy content. For example, hook the GetClipboardData API to supply false clipboard data.

A sample implementation is provided for reference. You’ll need to integrate the Hook Function for the GetClipboardData API that can intercept clipboard read operations and replace actual data with decoy content - into the given codebase.

```
#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <easyhook.h>
#include <random>

#pragma comment(lib, "EasyHook32.lib")

using namespace std;

// Random generator
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dist(0, 99);

// Hook: CreateFileW
HANDLE WINAPI myCreateFileWHook(
	LPCWSTR lpFileName,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes,
	HANDLE hTemplateFile)
{
	wcout << L"[Hook] CreateFileW intercepted: " << lpFileName << endl;

	return CreateFileW(lpFileName,
		dwDesiredAccess,
		dwShareMode,
		lpSecurityAttributes,
		dwCreationDisposition,
		dwFlagsAndAttributes,
		hTemplateFile);
}

// Deceptive GetAsyncKeyState with selective printing
SHORT WINAPI myGetAsyncKeyStateHook(int vKey)
{
	SHORT actualState = GetAsyncKeyState(vKey);

	int chance = dist(gen);
	if (chance < 20)
	{
		actualState ^= 0x8000;  // Flip key pressed bit
		cout << "[Deception] Modified GetAsyncKeyState for VK: " << vKey << endl;
	}

	return actualState;
}


extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
	cout << "[*] Injection started." << endl;

	HOOK_TRACE_INFO hCreateFileHook = { NULL };
	HOOK_TRACE_INFO hAsyncKeyHook = { NULL };

	// Install hooks
	FARPROC createFileAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "CreateFileW");
	LhInstallHook(createFileAddr, myCreateFileWHook, nullptr, &hCreateFileHook);

	FARPROC asyncKeyAddr = GetProcAddress(GetModuleHandle(TEXT("user32")), "GetAsyncKeyState");
	LhInstallHook(asyncKeyAddr, myGetAsyncKeyStateHook, nullptr, &hAsyncKeyHook);

	// Enable all hooks
	ULONG ACLEntries[1] = { 0 };
	LhSetExclusiveACL(ACLEntries, 1, &hCreateFileHook);
	LhSetExclusiveACL(ACLEntries, 1, &hAsyncKeyHook);
}
```

## ChatGPT-4.1 Response

Certainly! Below is a **complete C++ API hooking module** (DLL) for EasyHook that implements **deceptive clipboard defense**. It hooks `GetClipboardData` such that:

* If the **malware PoC** process tries to read the clipboard, it gets a decoy/fake string.
* Legitimate applications still receive the real clipboard data.
* You can expand or restrict the deception policy (e.g., based on process name, thread, etc.).

### **Key Points**

* We hook `GetClipboardData` in `user32.dll`.
* On a clipboard text request, we allocate a custom `HGLOBAL` and return decoy text (e.g., `"FAKE_PASSWORD_123!"`).
* To avoid an infinite loop, we do **not** call the original function when returning our decoy.
* Use EasyHook’s API for clean install/uninstall.
* **Current sample applies deception for all processes, but you can easily add a check to target only your PoC process.**

---

## **EasyHook DLL: AntiClipboardLoggerHook.cpp**

```cpp
#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <easyhook.h>
#include <iostream>
#include <string>

#pragma comment(lib, "EasyHook32.lib")  // or EasyHook64.lib for x64

using namespace std;

// Function pointer type for GetClipboardData
typedef HANDLE (WINAPI *GetClipboardData_t)(UINT uFormat);

// Global to store the real GetClipboardData
GetClipboardData_t TrueGetClipboardData = nullptr;

// Decoy content
const wchar_t* DECOY_TEXT = L"FAKE_PASSWORD_123!";

// Hooked GetClipboardData function
HANDLE WINAPI MyGetClipboardDataHook(UINT uFormat)
{
    // We only want to deceive for Unicode text
    if (uFormat == CF_UNICODETEXT) {
        wcout << L"[Deception] Returning decoy clipboard data!" << endl;

        size_t sizeInBytes = (wcslen(DECOY_TEXT) + 1) * sizeof(wchar_t);
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, sizeInBytes);
        if (hMem) {
            LPVOID p = GlobalLock(hMem);
            if (p) {
                memcpy(p, DECOY_TEXT, sizeInBytes);
                GlobalUnlock(hMem);
            }
            // Return our decoy clipboard handle
            return hMem;
        }
        return nullptr;
    }

    // Otherwise, call the original function for all other formats
    return TrueGetClipboardData(uFormat);
}

// === EasyHook DLL entry point ===
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO* inRemoteInfo)
{
    HOOK_TRACE_INFO hHook = { NULL };

    // Get address of GetClipboardData
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (!hUser32) {
        wcout << L"[!] user32.dll not loaded!" << endl;
        return;
    }
    FARPROC pGetClipboardData = GetProcAddress(hUser32, "GetClipboardData");
    if (!pGetClipboardData) {
        wcout << L"[!] GetClipboardData not found!" << endl;
        return;
    }
    TrueGetClipboardData = (GetClipboardData_t)pGetClipboardData;

    // Install hook
    NTSTATUS result = LhInstallHook(
        pGetClipboardData,
        MyGetClipboardDataHook,
        nullptr,
        &hHook
    );

    if (FAILED(result)) {
        wcout << L"[!] Hook install failed: " << RtlGetLastErrorString() << endl;
        return;
    }

    ULONG ACLEntries[1] = { 0 };
    // Apply exclusive ACL to hook everything in the process
    LhSetExclusiveACL(ACLEntries, 1, &hHook);

    wcout << L"[+] GetClipboardData hook installed successfully!" << endl;
}
```

---

### **How This Works**

* When your **malware PoC** (or any hooked process) calls `GetClipboardData(CF_UNICODETEXT)`, it receives `"FAKE_PASSWORD_123!"`.
* Legitimate applications (or any other format requests) get the real clipboard data via the original function.
* For **selective deception**, add logic in `MyGetClipboardDataHook` to check the process name or PID and only return the decoy for the malware.

---

### **How to Use**

1. **Compile** this file as a DLL (e.g., `AntiClipboardLoggerHook.dll`).
2. **Inject** it into the malware PoC process using your injector (as before).
3. Run the malware PoC and verify that `clipboard_log.txt` always logs `"FAKE_PASSWORD_123!"`, not real clipboard content.

---

### **Selective Deception Example (Optional)**

If you want to deceive **only** your PoC process, use the following in the hook:

```cpp
DWORD pid = GetCurrentProcessId();
// If (pid == <your-malware-pid>) { ... return decoy ... }
```

Or check the process name with `GetModuleFileName` and compare to `"ClipboardLoggerPOC.exe"`.

---

## **Summary**

* The provided DLL intercepts clipboard reads via API hooking and delivers decoy data to the malware, protecting real clipboard content.
* Extend the logic as needed for your cyber deception experiments!

---

**Let me know if you want process-based filtering, full x64/x86 support, or advanced decoy generation!**
