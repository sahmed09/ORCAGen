#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <easyhook.h>
#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <map>

#pragma comment(lib, "EasyHook32.lib")

using namespace std;

// Sensitive keywords/extension targets
const std::vector<std::string> kSensitiveExt = { ".json", ".conf", ".ini", ".txt", ".dat" };
const std::vector<std::string> kSensitiveSubstr = { "api_key", "wallet", "credential", "password", "secret", "config", "browser", "profile" };

// Decoy filenames to mislead malware
const std::vector<std::string> kDecoyFiles = {
	"not_a_password.txt",
	"wallet_backup.conf",
	"api_key_dummy.json",
	"secret_placeholder.dat",
	"browser_profile_fake.ini",
	"credentials_test.txt"
};

// Original function pointers
typedef HANDLE(WINAPI* PFN_FindFirstFileA)(LPCSTR, LPWIN32_FIND_DATAA);
typedef BOOL(WINAPI* PFN_FindNextFileA)(HANDLE, LPWIN32_FIND_DATAA);

PFN_FindFirstFileA TrueFindFirstFileA = FindFirstFileA;
PFN_FindNextFileA  TrueFindNextFileA = FindNextFileA;

// Internal state to serve decoy files
struct FakeFindHandle {
	size_t idx;
	std::vector<std::string> files;
};
std::map<HANDLE, FakeFindHandle> g_fakeHandles;

// Utility: checks if a filename looks "sensitive"
bool IsSensitive(const std::string& fname) {
	std::string lower = fname;
	std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
	for (const auto& ext : kSensitiveExt)
		if (lower.size() >= ext.size() && lower.compare(lower.size() - ext.size(), ext.size(), ext) == 0)
			return true;
	for (const auto& kw : kSensitiveSubstr)
		if (lower.find(kw) != std::string::npos)
			return true;
	return false;
}

// Hooked FindFirstFileA
HANDLE WINAPI myFindFirstFileAHook(LPCSTR lpFileName, LPWIN32_FIND_DATAA lpFindFileData)
{
	std::string mask = lpFileName ? lpFileName : "";
	bool looksMaliciousScan = false;

	// Very simple heuristic: malware scans directories with mask "*"
	if (mask.find("*") != std::string::npos) {
		looksMaliciousScan = true;
	}
	// Could check for known malware scan paths/filenames here.

	if (looksMaliciousScan) {
		// Serve decoy files
		static int handleSeed = 12345;
		HANDLE fakeHandle = (HANDLE)(++handleSeed);
		g_fakeHandles[fakeHandle] = { 0, kDecoyFiles };

		// Return first decoy
		strcpy_s(lpFindFileData->cFileName, MAX_PATH, kDecoyFiles[0].c_str());
		lpFindFileData->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
		return fakeHandle;
	}

	// Legitimate - call real API
	return TrueFindFirstFileA(lpFileName, lpFindFileData);
}

// Hooked FindNextFileA
BOOL WINAPI myFindNextFileAHook(HANDLE hFindFile, LPWIN32_FIND_DATAA lpFindFileData)
{
	auto it = g_fakeHandles.find(hFindFile);
	if (it != g_fakeHandles.end()) {
		// Serve next decoy, if any
		it->second.idx++;
		if (it->second.idx < it->second.files.size()) {
			strcpy_s(lpFindFileData->cFileName, MAX_PATH, it->second.files[it->second.idx].c_str());
			lpFindFileData->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
			return TRUE;
		}
		else {
			g_fakeHandles.erase(it);
			SetLastError(ERROR_NO_MORE_FILES);
			return FALSE;
		}
	}

	// Legitimate - call real API
	return TrueFindNextFileA(hFindFile, lpFindFileData);
}

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO * inRemoteInfo)
{
	std::cout << "[*] AntiFileScannerHook DLL injected!" << std::endl;

	HOOK_TRACE_INFO hFindFirstFileA = { NULL };
	HOOK_TRACE_INFO hFindNextFileA = { NULL };

	// Install the hooks
	FARPROC fFirst = GetProcAddress(GetModuleHandleA("kernel32.dll"), "FindFirstFileA");
	FARPROC fNext = GetProcAddress(GetModuleHandleA("kernel32.dll"), "FindNextFileA");
	LhInstallHook(fFirst, myFindFirstFileAHook, nullptr, &hFindFirstFileA);
	LhInstallHook(fNext, myFindNextFileAHook, nullptr, &hFindNextFileA);

	// Exclusive ACL: only affect hooked process (the malware)
	ULONG ACLEntries[1] = { 0 };
	LhSetExclusiveACL(ACLEntries, 1, &hFindFirstFileA);
	LhSetExclusiveACL(ACLEntries, 1, &hFindNextFileA);

	std::cout << "[*] File system API hooks installed." << std::endl;
}
