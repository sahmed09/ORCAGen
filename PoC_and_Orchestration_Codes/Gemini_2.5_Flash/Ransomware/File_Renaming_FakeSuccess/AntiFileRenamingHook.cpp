#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <easyhook.h>
#include <random>
#include <vector> // Required for std::vector
#include <algorithm> // Required for std::transform and std::tolower

#pragma comment(lib, "EasyHook32.lib") // or EasyHook64.lib depending on platform

using namespace std;

// Original function pointers
// This is crucial: we need to define a typedef for the original function signature
// and a global variable to store the pointer to the real function.
typedef BOOL(WINAPI* PFN_MoveFileExA)(LPCSTR lpExistingFileName, LPCSTR lpNewFileName, DWORD dwFlags);
PFN_MoveFileExA Real_MoveFileExA = NULL;

// List of suspicious file extensions (in lowercase for case-insensitive comparison)
// This can be expanded with more ransomware-specific extensions.
const vector<string> SUSPICIOUS_EXTENSIONS = {
	".locked",
	".enc",
	".revil",
	".wannacry",
	".crypt",
	".zepto",
	".aes256",
	".decryptme",
	".ransom"
};

// Helper function to check if a string ends with a specific suffix (pre-C++20 compatible)
bool endsWith(const std::string& str, const std::string& suffix) {
	if (str.length() >= suffix.length()) {
		return (0 == str.compare(str.length() - suffix.length(), suffix.length(), suffix));
	}
	return false;
}

// Hook function for MoveFileExA
// This function will be called instead of the original MoveFileExA
BOOL WINAPI myMoveFileExAHook(
	LPCSTR lpExistingFileName,
	LPCSTR lpNewFileName,
	DWORD dwFlags)
{
	cout << "[+] MoveFileExA Hook triggered!" << endl;
	cout << "  Original Path: " << (lpExistingFileName ? lpExistingFileName : "N/A") << endl;
	cout << "  New Path: " << (lpNewFileName ? lpNewFileName : "N/A") << endl;
	cout << "  Flags: " << hex << dwFlags << dec << endl;

	// Check if lpNewFileName is valid
	if (lpNewFileName != nullptr)
	{
		string newPathStr(lpNewFileName);

		// Convert newPathStr to lowercase for case-insensitive comparison
		transform(newPathStr.begin(), newPathStr.end(), newPathStr.begin(),
			[](unsigned char c) { return tolower(c); });

		// Extract the extension from the new file path
		size_t dotPos = newPathStr.rfind('.');
		if (dotPos != string::npos)
		{
			string extension = newPathStr.substr(dotPos);

			// Check if the extension is in our list of suspicious extensions
			for (const string& suspiciousExt : SUSPICIOUS_EXTENSIONS)
			{
				// Corrected: Use custom endsWith helper function for pre-C++20 compatibility
				if (extension == suspiciousExt || endsWith(newPathStr, suspiciousExt))
				{
					cout << "[!!!] DETECTED RANSOMWARE-LIKE RENAME ATTEMPT!" << endl;
					cout << "      Attempted to rename: '" << (lpExistingFileName ? lpExistingFileName : "N/A") << "' to suspicious: '" << (lpNewFileName ? lpNewFileName : "N/A") << "'" << endl;
					cout << "      Blocking this operation and sending fake success." << endl;

					// Deception: Block the operation by not calling the original function,
					// but return TRUE to simulate success to the malware.
					// This tricks the malware into thinking the file was renamed/encrypted,
					// while the original file remains untouched.
					// You might also log this to a file or send to a monitoring system here.
					return TRUE; // Fake success
				}
			}
		}
	}

	// If not a suspicious rename, call the original MoveFileExA function
	cout << "  Legitimate rename, allowing operation." << endl;
	return Real_MoveFileExA(lpExistingFileName, lpNewFileName, dwFlags);
}


// --- Existing Hooked functions from previous sample (Beep and GetAsyncKeyState) ---
DWORD gFreqOffset = 0; // Global for Beep hook, though not used in injector current state

BOOL WINAPI myBeepHook(DWORD dwFreq, DWORD dwDuration)
{
	cout << "[+] BeepHook triggered!" << endl;
	cout << "Original Frequency: " << dwFreq << ", Duration: " << dwDuration << endl;
	// This hook is for demonstration, not blocking.
	return Beep(dwFreq + gFreqOffset, dwDuration);
}

// Random generator for GetAsyncKeyState deception
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dist(0, 99);

// Deceptive GetAsyncKeyState with selective printing
SHORT WINAPI myGetAsyncKeyStateHook(int vKey)
{
	SHORT actualState = GetAsyncKeyState(vKey);

	int chance = dist(gen);
	if (chance < 20) // 20% chance to flip the pressed bit
	{
		actualState ^= 0x8000;  // Flip key pressed bit (if pressed, make it not; if not, make it pressed)
		cout << "[Deception] Modified GetAsyncKeyState for VK: " << vKey << " (Deceiving malware on key state)" << endl;
	}

	return actualState;
}

// --- NativeInjectionEntryPoint (DLL's main entry point) ---
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
	cout << "[*] AntiClipboardLoggerHook: Injection started." << endl;

	// --- Process UserData if any (for Beep hook, though not used by current injector) ---
	if (inRemoteInfo->UserDataSize == sizeof(DWORD))
	{
		gFreqOffset = *reinterpret_cast<DWORD*>(inRemoteInfo->UserData);
		cout << "Frequency Offset Received: " << gFreqOffset << endl;
	}

	// --- Hooks ---
	HOOK_TRACE_INFO hBeepHook = { NULL };
	HOOK_TRACE_INFO hAsyncKeyHook = { NULL };
	HOOK_TRACE_INFO hMoveFileExAHook = { NULL }; // New hook info for MoveFileExA

	NTSTATUS result;
	ULONG ACLEntries[1] = { 0 }; // ACL for all threads in the process

	// 1. Install Beep Hook (from previous example)
	FARPROC beepAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "Beep");
	if (beepAddr == NULL) { wcerr << L"Error: Could not find Beep address." << endl; }
	else {
		cout << "Beep function address: " << (void*)beepAddr << endl;
		result = LhInstallHook(beepAddr, myBeepHook, nullptr, &hBeepHook);
		if (FAILED(result)) {
			wstring s(RtlGetLastErrorString());
			wcerr << L"Failed to install Beep hook: " << s << endl;
		}
		else {
			cout << "Beep hook installed successfully!" << endl;
			// Enable the hook immediately after successful installation
			result = LhSetExclusiveACL(ACLEntries, 1, &hBeepHook);
			if (FAILED(result)) { wstring s(RtlGetLastErrorString()); wcerr << L"Failed to enable Beep hook: " << s << endl; }
		}
	}


	// 2. Install GetAsyncKeyState Hook (from previous example)
	FARPROC asyncKeyAddr = GetProcAddress(GetModuleHandle(TEXT("user32")), "GetAsyncKeyState");
	if (asyncKeyAddr == NULL) { wcerr << L"Error: Could not find GetAsyncKeyState address." << endl; }
	else {
		cout << "GetAsyncKeyState function address: " << (void*)asyncKeyAddr << endl;
		result = LhInstallHook(asyncKeyAddr, myGetAsyncKeyStateHook, nullptr, &hAsyncKeyHook);
		if (FAILED(result)) {
			wstring s(RtlGetLastErrorString());
			wcerr << L"Failed to install GetAsyncKeyState hook: " << s << endl;
		}
		else {
			cout << "GetAsyncKeyState hook installed successfully!" << endl;
			// Enable the hook immediately after successful installation
			result = LhSetExclusiveACL(ACLEntries, 1, &hAsyncKeyHook);
			if (FAILED(result)) { wstring s(RtlGetLastErrorString()); wcerr << L"Failed to enable GetAsyncKeyState hook: " << s << endl; }
		}
	}

	// 3. Install MoveFileExA Hook (NEW)
	FARPROC moveFileExAAddr = GetProcAddress(GetModuleHandleA("kernel32.dll"), "MoveFileExA");
	if (moveFileExAAddr == NULL) { wcerr << L"Error: Could not find MoveFileExA address." << endl; }
	else {
		cout << "MoveFileExA function address: " << (void*)moveFileExAAddr << endl;
		// Store the real function pointer BEFORE installing the hook
		Real_MoveFileExA = reinterpret_cast<PFN_MoveFileExA>(moveFileExAAddr);

		result = LhInstallHook(moveFileExAAddr, myMoveFileExAHook, nullptr, &hMoveFileExAHook);
		if (FAILED(result)) {
			wstring s(RtlGetLastErrorString());
			wcerr << L"Failed to install MoveFileExA hook: " << s << endl;
		}
		else {
			cout << "MoveFileExA hook installed successfully!" << endl;
			// Enable the hook immediately after successful installation
			result = LhSetExclusiveACL(ACLEntries, 1, &hMoveFileExAHook);
			if (FAILED(result)) { wstring s(RtlGetLastErrorString()); wcerr << L"Failed to enable MoveFileExA hook: " << s << endl; }
		}
	}

	cout << "[*] AntiClipboardLoggerHook: Injection and hooks setup complete. Waiting for target process exit..." << endl;
	// Keep the DLL loaded until the target process exits.
	// This is important because if this function returns, the DLL might be unloaded,
	// and the hooks would be removed.
	RhWakeUpProcess();
}
