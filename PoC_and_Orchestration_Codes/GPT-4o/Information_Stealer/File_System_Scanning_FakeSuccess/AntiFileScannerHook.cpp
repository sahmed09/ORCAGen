#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <easyhook.h>
#include <iostream>
#include <string>
#include <vector>

#pragma comment(lib, "EasyHook32.lib")

using namespace std;

typedef HANDLE(WINAPI* FindFirstFileW_t)(LPCWSTR, LPWIN32_FIND_DATAW);
typedef BOOL(WINAPI* FindNextFileW_t)(HANDLE, LPWIN32_FIND_DATAW);

FindFirstFileW_t originalFindFirstFileW = nullptr;
FindNextFileW_t originalFindNextFileW = nullptr;

HANDLE g_fakeHandle = (HANDLE)0xDEADBEEF;
bool g_returnedDecoy = false;

std::vector<std::wstring> g_decoys = {
	L"wallet_mock.dat", L"config_fake.txt", L"api_key_dummy.json", L"password_list.conf"
};

HANDLE WINAPI myFindFirstFileW(LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData)
{
	wcout << L"[Hook] FindFirstFileW intercepted for: " << lpFileName << endl;

	if (wcsstr(lpFileName, L"MalwareEvaluation-main") != nullptr)
	{
		g_returnedDecoy = false;
		wcscpy_s(lpFindFileData->cFileName, MAX_PATH, g_decoys[0].c_str());
		return g_fakeHandle;
	}

	return originalFindFirstFileW(lpFileName, lpFindFileData);
}

/*HANDLE WINAPI myFindFirstFileW(LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData)
{
	wcout << L"[Hook] FindFirstFileW intercepted for: " << lpFileName << endl;

	if (wcsstr(lpFileName, L"MalwareEvaluation-main") != nullptr)
	{
		g_returnedDecoy = false;
		wcscpy_s(lpFindFileData->cFileName, MAX_PATH, g_decoys[0].c_str());

		wcout << L"[Hook] Returning decoy (TP): " << lpFindFileData->cFileName << endl;
		return g_fakeHandle;
	}

	// Log non-decoy result
	wcout << L"[Hook] No match. Returning real file (TN or FN)" << endl;
	return originalFindFirstFileW(lpFileName, lpFindFileData);
}*/


BOOL WINAPI myFindNextFileW(HANDLE hFindFile, LPWIN32_FIND_DATAW lpFindFileData)
{
	if (hFindFile == g_fakeHandle)
	{
		if (!g_returnedDecoy && g_decoys.size() > 1)
		{
			wcscpy_s(lpFindFileData->cFileName, MAX_PATH, g_decoys[1].c_str());
			g_returnedDecoy = true;
			return TRUE;
		}

		SetLastError(ERROR_NO_MORE_FILES);
		return FALSE;
	}

	return originalFindNextFileW(hFindFile, lpFindFileData);
}

/*BOOL WINAPI myFindNextFileW(HANDLE hFindFile, LPWIN32_FIND_DATAW lpFindFileData)
{
	if (hFindFile == g_fakeHandle)
	{
		if (!g_returnedDecoy && g_decoys.size() > 1)
		{
			wcscpy_s(lpFindFileData->cFileName, MAX_PATH, g_decoys[1].c_str());
			g_returnedDecoy = true;
			wcout << L"[Hook] Returning additional decoy (TP): " << lpFindFileData->cFileName << endl;
			return TRUE;
		}

		SetLastError(ERROR_NO_MORE_FILES);
		return FALSE;
	}

	wcout << L"[Hook] Real FindNextFileW call (possibly TN or FN)" << endl;
	return originalFindNextFileW(hFindFile, lpFindFileData);
}*/


extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(REMOTE_ENTRY_INFO* inRemoteInfo)
{
	std::wcout << L"[*] AntiFileScannerHook injection started." << std::endl;

	HOOK_TRACE_INFO hFindFirstHook = { NULL };
	HOOK_TRACE_INFO hFindNextHook = { NULL };

	originalFindFirstFileW = (FindFirstFileW_t)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "FindFirstFileW");
	LhInstallHook((PVOID)originalFindFirstFileW, (PVOID)myFindFirstFileW, nullptr, &hFindFirstHook);

	originalFindNextFileW = (FindNextFileW_t)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "FindNextFileW");
	LhInstallHook((PVOID)originalFindNextFileW, (PVOID)myFindNextFileW, nullptr, &hFindNextHook);

	ULONG ACLEntries[1] = { 0 };
	LhSetExclusiveACL(ACLEntries, 1, &hFindFirstHook);
	LhSetExclusiveACL(ACLEntries, 1, &hFindNextHook);

	std::wcout << L"[+] Hooks installed successfully." << std::endl;
}
