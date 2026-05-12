#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <easyhook.h>
#include <random>
#include <algorithm> // For std::transform
#include <cwctype>   // For ::towlower

#pragma comment(lib, "EasyHook32.lib") // or EasyHook64.lib depending on platform

using namespace std;

// Original function pointers
// Define a typedef for the original FindFirstFileW function signature
// CORRECTED: LPWIN33_FIND_DATAW changed to LPWIN32_FIND_DATAW
typedef HANDLE(WINAPI* PFN_FindFirstFileW)(LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData);
PFN_FindFirstFileW g_pOriginalFindFirstFileW = nullptr;

// Define a typedef for the original FindNextFileW function signature
// CORRECTED: LPWIN33_FIND_DATAW changed to LPWIN32_FIND_DATAW
typedef BOOL(WINAPI* PFN_FindNextFileW)(HANDLE hFindFile, LPWIN32_FIND_DATAW lpFindFileData);
PFN_FindNextFileW g_pOriginalFindNextFileW = nullptr;

// Random generator for decoy selection
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dist(0, 9); // For selecting decoy files

// Decoy filenames to present to the malware
const std::vector<std::wstring> decoyFilenames = {
	L"dummy_config.txt",
	L"temp_log.log",
	L"backup_data.bak",
	L"image001.jpg",
	L"readme.md",
	L"installer.exe",
	L"document.pdf",
	L"notes.txt",
	L"system_info.nfo",
	L"report.doc"
};

// Sensitive file patterns (extensions and filenames) - copied from malware POC
const std::vector<std::wstring> sensitiveExtensions = {
	L".json", L".conf", L".ini", L".txt", L".log", L".bak",
	L".key", L".pem", L".crt", L".cer", L".pfx", L".gpg", L".asc",
	L".wallet", L".dat", L".db", L".sqlite", L".xml", L".yml", L".yaml"
};

const std::vector<std::wstring> sensitiveFilenames = {
	L"credentials", L"api_key", L"id_rsa", L"wallet.dat", L"config",
	L"secrets", L"private_key", L"public_key", L"authorization",
	L"token", L"password", L"passwd", L"history", L"bookmarks", L"cookies"
};

// Function to check if a filename is sensitive (replicated from malware POC)
// This helps us identify when the malware is looking for a sensitive file.
bool isSensitiveFileCheck(const std::wstring& filename) {
	// Check by exact filename match
	for (const auto& sensitiveName : sensitiveFilenames) {
		if (_wcsicmp(filename.c_str(), sensitiveName.c_str()) == 0) {
			return true;
		}
	}

	// Check by extension
	size_t dotPos = filename.find_last_of(L'.');
	if (dotPos != std::wstring::npos) {
		std::wstring ext = filename.substr(dotPos);
		for (const auto& sensitiveExt : sensitiveExtensions) {
			if (_wcsicmp(ext.c_str(), sensitiveExt.c_str()) == 0) {
				return true;
			}
		}
	}

	// Check if any part of the filename contains a sensitive keyword
	std::wstring lowerFilename = filename;
	std::transform(lowerFilename.begin(), lowerFilename.end(), lowerFilename.begin(), ::towlower);
	for (const auto& sensitiveName : sensitiveFilenames) {
		std::wstring lowerSensitiveName = sensitiveName;
		std::transform(lowerSensitiveName.begin(), lowerSensitiveName.end(), lowerSensitiveName.begin(), ::towlower);
		if (lowerFilename.find(lowerSensitiveName) != std::wstring::npos) {
			return true;
		}
	}
	return false;
}


// --- HOOKED FUNCTIONS ---

// Hook for FindFirstFileW
HANDLE WINAPI Hook_FindFirstFileW(LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData)
{
	wcout << L"[Hook] FindFirstFileW intercepted for: " << lpFileName << endl;

	// Call the original function first to get real file data
	HANDLE hRealFind = g_pOriginalFindFirstFileW(lpFileName, lpFindFileData);

	if (hRealFind != INVALID_HANDLE_VALUE) {
		// Simulate delay
		// Sleep(50);
		
		// Create a decoy file entry
		memset(lpFindFileData, 0, sizeof(WIN32_FIND_DATAW)); // Clear real data

		// Pick a random decoy filename
		int decoyIndex = dist(gen);
		if (decoyIndex >= decoyFilenames.size()) decoyIndex = 0; // Fallback

		wcsncpy_s(lpFindFileData->cFileName, MAX_PATH, decoyFilenames[decoyIndex].c_str(), _TRUNCATE);
		lpFindFileData->dwFileAttributes = FILE_ATTRIBUTE_ARCHIVE; // Common attribute for normal files
		lpFindFileData->nFileSizeLow = (decoyIndex + 1) * 1024; // Dummy file size (e.g., 1KB, 2KB, etc.)

		wcout << L"    [Deception] Returning decoy file for first entry: " << lpFindFileData->cFileName << endl;
		// Return a valid handle, even if we replaced the data.
		// The malware will then proceed with FindNextFileW.
		return hRealFind;
	}
	else {
		// If the original call failed (e.g., directory not found), propagate the error
		wcout << L"    [Hook] Original FindFirstFileW failed. Error: " << GetLastError() << endl;
		return INVALID_HANDLE_VALUE;
	}
}

// Hook for FindNextFileW
BOOL WINAPI Hook_FindNextFileW(HANDLE hFindFile, LPWIN32_FIND_DATAW lpFindFileData)
{
	// Call the original function first to get real file data
	BOOL bResult = g_pOriginalFindNextFileW(hFindFile, lpFindFileData);

	if (bResult) {
		// Simulate delay
		// Sleep(50);
		
		// If the original call found a file, we can replace it with a decoy
		// This is where we consistently return decoy files

		// Pick a random decoy filename
		int decoyIndex = dist(gen);
		if (decoyIndex >= decoyFilenames.size()) decoyIndex = 0; // Fallback

		memset(lpFindFileData, 0, sizeof(WIN32_FIND_DATAW)); // Clear real data before populating with decoy
		wcsncpy_s(lpFindFileData->cFileName, MAX_PATH, decoyFilenames[decoyIndex].c_str(), _TRUNCATE);
		lpFindFileData->dwFileAttributes = FILE_ATTRIBUTE_ARCHIVE; // Common attribute for normal files
		lpFindFileData->nFileSizeLow = (decoyIndex + 1) * 1024; // Dummy file size

		wcout << L"    [Deception] Returning decoy file for next entry: " << lpFindFileData->cFileName << endl;
	}
	else {
		// If no more real files, the original function will return FALSE, and we should too.
		wcout << L"    [Hook] Original FindNextFileW returned no more files or an error." << endl;
	}

	return bResult;
}


extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
	cout << "[*] AntiClipboardLoggerHook: Injection started." << endl;

	HOOK_TRACE_INFO hFindFirstFileW = { NULL };
	HOOK_TRACE_INFO hFindNextFileW = { NULL };

	// Get original function addresses
	g_pOriginalFindFirstFileW = (PFN_FindFirstFileW)GetProcAddress(GetModuleHandle(TEXT("kernel32")), "FindFirstFileW");
	g_pOriginalFindNextFileW = (PFN_FindNextFileW)GetProcAddress(GetModuleHandle(TEXT("kernel32")), "FindNextFileW");

	if (g_pOriginalFindFirstFileW == nullptr || g_pOriginalFindNextFileW == nullptr) {
		wcerr << L"[-] Failed to get address of FindFirstFileW or FindNextFileW." << endl;
		return;
	}
	wcout << L"    FindFirstFileW original address: " << (void*)g_pOriginalFindFirstFileW << endl;
	wcout << L"    FindNextFileW original address: " << (void*)g_pOriginalFindNextFileW << endl;

	// Install hooks
	NTSTATUS resultFindFirst = LhInstallHook(
		g_pOriginalFindFirstFileW,
		Hook_FindFirstFileW,
		nullptr,
		&hFindFirstFileW
	);

	NTSTATUS resultFindNext = LhInstallHook(
		g_pOriginalFindNextFileW,
		Hook_FindNextFileW,
		nullptr,
		&hFindNextFileW
	);

	if (FAILED(resultFindFirst))
	{
		wstring s(RtlGetLastErrorString());
		wcerr << L"[-] Failed to install FindFirstFileW hook: " << s << endl;
	}
	else
	{
		cout << "[+] FindFirstFileW hook installed successfully!" << endl;
	}

	if (FAILED(resultFindNext))
	{
		wstring s(RtlGetLastErrorString());
		wcerr << L"[-] Failed to install FindNextFileW hook: " << s << endl;
	}
	else
	{
		cout << "[+] FindNextFileW hook installed successfully!" << endl;
	}

	// Enable all hooks for all threads in the target process
	ULONG ACLEntries[1] = { 0 };
	if (SUCCEEDED(resultFindFirst)) {
		LhSetExclusiveACL(ACLEntries, 1, &hFindFirstFileW);
	}
	if (SUCCEEDED(resultFindNext)) {
		LhSetExclusiveACL(ACLEntries, 1, &hFindNextFileW);
	}

	// Keep the DLL loaded in the process
	while (true)
	{
		Sleep(1000);
	}
}