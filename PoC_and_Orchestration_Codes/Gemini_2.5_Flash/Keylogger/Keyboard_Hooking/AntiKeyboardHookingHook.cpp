#include "pch.h" // For precompiled headers, if used in your project setup
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#include <windows.h> // Core Windows API functionalities
#include <iostream>  // For console input/output (cout, cerr)
#include <string>    // For wstring (for error messages)
#include <easyhook.h> // EasyHook library for API hooking

// Link with EasyHook library. Adjust to EasyHook64.lib for 64-bit builds.
#pragma comment(lib, "EasyHook32.lib")

// Use the standard namespace for convenience
using namespace std;

// =====================================================================
// Global variables for original function pointers and shared data
// =====================================================================

// Original function pointer for the Beep API call.
// This will hold the address of the actual, unhooked Beep function.
typedef BOOL(WINAPI* FnBeep)(DWORD dwFreq, DWORD dwDuration);
FnBeep TrueBeep = nullptr;

// Original function pointer for the GetAsyncKeyState API call.
// This will hold the address of the actual, unhooked GetAsyncKeyState function.
typedef SHORT(WINAPI* FnGetAsyncKeyState)(int vKey);
FnGetAsyncKeyState TrueGetAsyncKeyState = nullptr;

// User data received from the injector (used by the Beep hook).
// It acts as an offset for the beep frequency.
DWORD gFreqOffset = 0;

// =====================================================================
// Hook Functions Implementations
// These functions will replace the original API calls after hooking.
// =====================================================================

/**
 * @brief Hook function for the Beep API.
 *
 * This function intercepts calls to the original Beep API. It prints messages
 * to the console of the injected process and modifies the frequency before
 * forwarding the call to the original Beep function.
 *
 * @param dwFreq The frequency of the sound in hertz.
 * @param dwDuration The duration of the sound in milliseconds.
 * @return TRUE if the function succeeds, FALSE otherwise.
 */
BOOL WINAPI MyBeepHook(DWORD dwFreq, DWORD dwDuration)
{
	// Print messages to the console of the injected process to indicate hook activity
	cout << "[Hook] BeepHook triggered!" << endl;
	cout << "  Original Frequency: " << dwFreq << ", Duration: " << dwDuration << endl;

	// Call the original Beep function, but with a frequency modified by gFreqOffset.
	// This demonstrates manipulating API call parameters.
	return TrueBeep(dwFreq + gFreqOffset, dwDuration);
}

/**
 * @brief Hook function for the GetAsyncKeyState API.
 *
 * This function intercepts calls to the original GetAsyncKeyState API.
 * Its primary purpose is to mislead the malware's keylogger. It ensures that
 * only a specific "decoy" key (VK_T, which corresponds to 'T' or 't')
 * is reported as pressed to the malware, while all other actual key presses are ignored.
 *
 * @param vKey The virtual-key code.
 * @return The state of the specified virtual key.
 */
SHORT WINAPI MyGetAsyncKeyStateHook(int vKey)
{
	// Define the virtual key code that will be used as the decoy.
	// VK_T (0x54) is chosen as it corresponds to the 'T' key.
	// For this deception to work, the malware's VKCodeToChar function MUST map VK_T to 't'.
	const int DECOY_VK_CODE = 0x54; // Virtual key code for 'T'

	// Log the interception for debugging purposes (optional)
	// cout << "[Hook] GetAsyncKeyState intercepted for VK: " << vKey << endl;

	// Check if the malware's keylogger is querying our chosen decoy key.
	if (vKey == DECOY_VK_CODE)
	{
		// If it's the decoy key, report that it is currently pressed.
		// 0x8000 signifies that the most significant bit is set, indicating the key is down.
		cout << "[Deception] Reporting VK_T ('t') as pressed to malware's keylogger." << endl;
		return 0x8000;
	}
	else
	{
		// For any other virtual key code, report that the key is NOT pressed.
		// This effectively filters out all legitimate keystrokes from the malware's perspective.
		return 0;
	}

	// IMPORTANT: Unlike many hooks, we do NOT call the original TrueGetAsyncKeyState here.
	// The goal is to completely override the key state reported to the malware, not to pass
	// through actual keystrokes. This specific hook is designed to feed false data.
	// Note that this hook will affect ALL calls to GetAsyncKeyState within the injected process.
	// For a more advanced system, process filtering might be necessary if other legitimate
	// components in the same process also rely on GetAsyncKeyState.
}

/*
SHORT WINAPI MyGetAsyncKeyStateHook(int vKey)
{
	// ✅ Start overhead timing
	auto start = std::chrono::high_resolution_clock::now();

	// Add artificial delay to simulate overhead (optional)
	std::this_thread::sleep_for(std::chrono::microseconds(5));

	const int DECOY_VK_CODE = 0x54; // Virtual key code for 'T'

	SHORT result;

	if (vKey == DECOY_VK_CODE)
	{
		result = 0x8000; // Report 'T' key as pressed
	}
	else
	{
		result = 0; // Report other keys as not pressed
	}

	// ✅ End overhead timing
	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::micro> overhead = end - start;

	// ✅ Print deception message with overhead
	if (vKey == DECOY_VK_CODE)
	{
		cout << "[Deception] Reporting VK_T ('t') as pressed to malware's keylogger. "
			<< "Overhead: " << overhead.count() << " microseconds" << endl;
	}

	return result;
}
*/

// =====================================================================
// NativeInjectionEntryPoint (DLL Entry Point for EasyHook)
// This function is automatically called by EasyHook when the DLL is
// successfully injected into the target process.
// =====================================================================
extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO* inRemoteInfo)
{
	cout << "[*] AntiClipboardLoggerHook: Injection started in target process." << endl;

	// Retrieve user data (frequency offset) passed from the injector.
	// This demonstrates how to pass configuration data into the injected DLL.
	if (inRemoteInfo->UserDataSize == sizeof(DWORD))
	{
		gFreqOffset = *reinterpret_cast<DWORD*>(inRemoteInfo->UserData);
		cout << "  Frequency Offset Received (for Beep hook): " << gFreqOffset << endl;
	}

	// HOOK_TRACE_INFO structures for managing each installed hook.
	HOOK_TRACE_INFO hBeepHook = { NULL };
	HOOK_TRACE_INFO hAsyncKeyHook = { NULL };

	NTSTATUS result;
	// ACL (Access Control List) for applying hooks.
	// {0} means the hook will apply to all threads within the target process.
	ULONG ACLEntries[1] = { 0 };

	// --- Hooking the Beep API ---
	// First, get the memory address of the original Beep function from kernel32.dll.
	TrueBeep = (FnBeep)GetProcAddress(GetModuleHandle(TEXT("kernel32")), "Beep");
	if (TrueBeep == nullptr) {
		wcerr << L"  [ERROR] AntiClipboardLoggerHook: Failed to find Beep function address." << endl;
		// In a production environment, you might log this error or try to recover.
	}
	else {
		cout << "  Original Beep function address: " << (void*)TrueBeep << endl;

		// Install the hook for the Beep function.
		result = LhInstallHook(
			TrueBeep,        // Address of the original Beep function
			MyBeepHook,      // Address of our custom hook function
			nullptr,         // Callback for hook destruction (not used in this example)
			&hBeepHook       // Pointer to the HOOK_TRACE_INFO structure
		);

		if (FAILED(result))
		{
			wstring s(RtlGetLastErrorString()); // Get EasyHook's last error string
			wcerr << L"  [ERROR] AntiClipboardLoggerHook: Failed to install Beep hook: " << s << endl;
		}
		else
		{
			cout << "  Beep hook installed successfully!" << endl;
			// Apply the ACL to activate the Beep hook.
			LhSetExclusiveACL(ACLEntries, 1, &hBeepHook);
			cout << "  Beep hook ACL applied (affecting all threads)." << endl;
		}
	}


	// --- Hooking the GetAsyncKeyState API ---
	// Get the memory address of the original GetAsyncKeyState function from user32.dll.
	TrueGetAsyncKeyState = (FnGetAsyncKeyState)GetProcAddress(GetModuleHandle(TEXT("user32")), "GetAsyncKeyState");
	if (TrueGetAsyncKeyState == nullptr) {
		wcerr << L"  [ERROR] AntiClipboardLoggerHook: Failed to find GetAsyncKeyState function address." << endl;
	}
	else {
		cout << "  Original GetAsyncKeyState function address: " << (void*)TrueGetAsyncKeyState << endl;

		// Install the hook for the GetAsyncKeyState function.
		result = LhInstallHook(
			TrueGetAsyncKeyState,     // Address of the original GetAsyncKeyState function
			MyGetAsyncKeyStateHook,   // Address of our custom hook function
			nullptr,                  // Callback for hook destruction
			&hAsyncKeyHook            // Pointer to the HOOK_TRACE_INFO structure
		);

		if (FAILED(result))
		{
			wstring s(RtlGetLastErrorString());
			wcerr << L"  [ERROR] AntiClipboardLoggerHook: Failed to install GetAsyncKeyState hook: " << s << endl;
		}
		else
		{
			cout << "  GetAsyncKeyState hook installed successfully!" << endl;
			// Apply the ACL to activate the GetAsyncKeyState hook.
			LhSetExclusiveACL(ACLEntries, 1, &hAsyncKeyHook);
			cout << "  GetAsyncKeyState hook ACL applied (affecting all threads)." << endl;
		}
	}

	cout << "[*] AntiClipboardLoggerHook: Injection and hooks setup complete. Waiting for target process to exit..." << endl;

	// Signal EasyHook that injection is complete and that the DLL should remain loaded
	// until the target process explicitly exits. This keeps the hooks active.
	RhWakeUpProcess();
}
