#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <easyhook.h>

#pragma comment(lib, "EasyHook32.lib") // Use EasyHook64.lib for x64

using namespace std;

HOOK_TRACE_INFO hFindFirstFileA = { NULL };
HOOK_TRACE_INFO hFindNextFileA = { NULL };
HOOK_TRACE_INFO hFindFirstFileW = { NULL };
HOOK_TRACE_INFO hFindNextFileW = { NULL };
HOOK_TRACE_INFO hCreateFileA = { NULL };
HOOK_TRACE_INFO hCreateFileW = { NULL };
HOOK_TRACE_INFO hReadFile = { NULL };

string gDecoyFileA = "decoy_credentials.txt";
wstring gDecoyFileW = L"decoy_credentials.txt";

int gDecoyIndexA = 0;
int gDecoyIndexW = 0;

vector<string> decoyNamesA =
{
	"api_key_backup.json",
	"wallet.dat",
	"browser_profile.ini",
	"credentials.conf",
	"token_cache.txt",
	"passwords_config.xml"
};

vector<wstring> decoyNamesW =
{
	L"api_key_backup.json",
	L"wallet.dat",
	L"browser_profile.ini",
	L"credentials.conf",
	L"token_cache.txt",
	L"passwords_config.xml"
};

string ToLowerA(string s)
{
	transform(s.begin(), s.end(), s.begin(), ::tolower);
	return s;
}

wstring ToLowerW(wstring s)
{
	transform(s.begin(), s.end(), s.begin(), towlower);
	return s;
}

bool EndsWithA(const string& value, const string& suffix)
{
	if (suffix.size() > value.size())
		return false;

	return equal(suffix.rbegin(), suffix.rend(), value.rbegin(),
		[](char a, char b)
		{
			return tolower(a) == tolower(b);
		});
}

bool EndsWithW(const wstring& value, const wstring& suffix)
{
	if (suffix.size() > value.size())
		return false;

	return equal(suffix.rbegin(), suffix.rend(), value.rbegin(),
		[](wchar_t a, wchar_t b)
		{
			return towlower(a) == towlower(b);
		});
}

bool IsSensitiveNameA(const string& name)
{
	string lower = ToLowerA(name);

	vector<string> extensions =
	{
		".json", ".conf", ".ini", ".txt", ".cfg", ".dat", ".log", ".xml"
	};

	vector<string> keywords =
	{
		"api_key", "apikey", "secret", "credential", "credentials",
		"password", "passwd", "wallet", "wallet.dat", "browser",
		"profile", "token", "config"
	};

	for (const auto& ext : extensions)
	{
		if (EndsWithA(lower, ext))
			return true;
	}

	for (const auto& key : keywords)
	{
		if (lower.find(key) != string::npos)
			return true;
	}

	return false;
}

bool IsSensitiveNameW(const wstring& name)
{
	wstring lower = ToLowerW(name);

	vector<wstring> extensions =
	{
		L".json", L".conf", L".ini", L".txt", L".cfg", L".dat", L".log", L".xml"
	};

	vector<wstring> keywords =
	{
		L"api_key", L"apikey", L"secret", L"credential", L"credentials",
		L"password", L"passwd", L"wallet", L"wallet.dat", L"browser",
		L"profile", L"token", L"config"
	};

	for (const auto& ext : extensions)
	{
		if (EndsWithW(lower, ext))
			return true;
	}

	for (const auto& key : keywords)
	{
		if (lower.find(key) != wstring::npos)
			return true;
	}

	return false;
}

void ReplaceWithDecoyA(WIN32_FIND_DATAA* data)
{
	if (!data)
		return;

	if (data->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		return;

	string original = data->cFileName;

	if (!IsSensitiveNameA(original))
		return;

	string decoy = decoyNamesA[gDecoyIndexA++ % decoyNamesA.size()];

	cout << "[Deception] Replacing discovered file: "
		<< original << " -> " << decoy << endl;

	strcpy_s(data->cFileName, MAX_PATH, decoy.c_str());
	strcpy_s(data->cAlternateFileName, 14, "");
}

void ReplaceWithDecoyW(WIN32_FIND_DATAW* data)
{
	if (!data)
		return;

	if (data->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		return;

	wstring original = data->cFileName;

	if (!IsSensitiveNameW(original))
		return;

	wstring decoy = decoyNamesW[gDecoyIndexW++ % decoyNamesW.size()];

	wcout << L"[Deception] Replacing discovered file: "
		<< original << L" -> " << decoy << endl;

	wcscpy_s(data->cFileName, MAX_PATH, decoy.c_str());
	wcscpy_s(data->cAlternateFileName, 14, L"");
}

HANDLE WINAPI myFindFirstFileAHook(
	LPCSTR lpFileName,
	LPWIN32_FIND_DATAA lpFindFileData)
{
	cout << "[Hook] FindFirstFileA: " << lpFileName << endl;

	HANDLE hFind = FindFirstFileA(lpFileName, lpFindFileData);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		ReplaceWithDecoyA(lpFindFileData);
	}

	return hFind;
}

BOOL WINAPI myFindNextFileAHook(
	HANDLE hFindFile,
	LPWIN32_FIND_DATAA lpFindFileData)
{
	BOOL result = FindNextFileA(hFindFile, lpFindFileData);

	if (result)
	{
		ReplaceWithDecoyA(lpFindFileData);
	}

	return result;
}

HANDLE WINAPI myFindFirstFileWHook(
	LPCWSTR lpFileName,
	LPWIN32_FIND_DATAW lpFindFileData)
{
	wcout << L"[Hook] FindFirstFileW: " << lpFileName << endl;

	HANDLE hFind = FindFirstFileW(lpFileName, lpFindFileData);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		ReplaceWithDecoyW(lpFindFileData);
	}

	return hFind;
}

BOOL WINAPI myFindNextFileWHook(
	HANDLE hFindFile,
	LPWIN32_FIND_DATAW lpFindFileData)
{
	BOOL result = FindNextFileW(hFindFile, lpFindFileData);

	if (result)
	{
		ReplaceWithDecoyW(lpFindFileData);
	}

	return result;
}

HANDLE WINAPI myCreateFileAHook(
	LPCSTR lpFileName,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes,
	HANDLE hTemplateFile)
{
	if (lpFileName != nullptr)
	{
		string fileName = lpFileName;

		if (IsSensitiveNameA(fileName))
		{
			cout << "[Deception] CreateFileA redirected: "
				<< fileName << " -> " << gDecoyFileA << endl;

			lpFileName = gDecoyFileA.c_str();
		}
	}

	return CreateFileA(
		lpFileName,
		dwDesiredAccess,
		dwShareMode,
		lpSecurityAttributes,
		dwCreationDisposition,
		dwFlagsAndAttributes,
		hTemplateFile
	);
}

HANDLE WINAPI myCreateFileWHook(
	LPCWSTR lpFileName,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes,
	HANDLE hTemplateFile)
{
	if (lpFileName != nullptr)
	{
		wstring fileName = lpFileName;

		if (IsSensitiveNameW(fileName))
		{
			wcout << L"[Deception] CreateFileW redirected: "
				<< fileName << L" -> " << gDecoyFileW << endl;

			lpFileName = gDecoyFileW.c_str();
		}
	}

	return CreateFileW(
		lpFileName,
		dwDesiredAccess,
		dwShareMode,
		lpSecurityAttributes,
		dwCreationDisposition,
		dwFlagsAndAttributes,
		hTemplateFile
	);
}

BOOL WINAPI myReadFileHook(
	HANDLE hFile,
	LPVOID lpBuffer,
	DWORD nNumberOfBytesToRead,
	LPDWORD lpNumberOfBytesRead,
	LPOVERLAPPED lpOverlapped)
{
	BOOL result = ReadFile(
		hFile,
		lpBuffer,
		nNumberOfBytesToRead,
		lpNumberOfBytesRead,
		lpOverlapped
	);

	if (result && lpNumberOfBytesRead && *lpNumberOfBytesRead > 0)
	{
		cout << "[Hook] ReadFile allowed. Bytes read: "
			<< *lpNumberOfBytesRead << endl;
	}

	return result;
}

void CreateDecoyFile()
{
	HANDLE hFile = CreateFileA(
		gDecoyFileA.c_str(),
		GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		cout << "[-] Failed to create decoy file." << endl;
		return;
	}

	string content =
		"{\r\n"
		"  \"api_key\": \"DECOY-API-KEY-12345\",\r\n"
		"  \"wallet_seed\": \"DECOY-WALLET-SEED\",\r\n"
		"  \"username\": \"fake_user\",\r\n"
		"  \"password\": \"fake_password\"\r\n"
		"}\r\n";

	DWORD written = 0;
	WriteFile(hFile, content.c_str(), (DWORD)content.size(), &written, NULL);
	CloseHandle(hFile);

	cout << "[+] Decoy file initialized: " << gDecoyFileA << endl;
}

void InstallHook(FARPROC target, PVOID hookFunc, HOOK_TRACE_INFO* hookInfo, const char* name)
{
	NTSTATUS result = LhInstallHook(target, hookFunc, nullptr, hookInfo);

	if (FAILED(result))
	{
		wcout << L"[-] Failed to install hook for " << name
			<< L": " << RtlGetLastErrorString() << endl;
	}
	else
	{
		cout << "[+] Hook installed: " << name << endl;
	}

	ULONG ACLEntries[1] = { 0 };
	LhSetExclusiveACL(ACLEntries, 1, hookInfo);
}

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(
	REMOTE_ENTRY_INFO * inRemoteInfo)
{
	cout << "[*] File system deception DLL injected." << endl;

	CreateDecoyFile();

	HMODULE kernel32 = GetModuleHandleA("kernel32.dll");

	InstallHook(
		GetProcAddress(kernel32, "FindFirstFileA"),
		myFindFirstFileAHook,
		&hFindFirstFileA,
		"FindFirstFileA"
	);

	InstallHook(
		GetProcAddress(kernel32, "FindNextFileA"),
		myFindNextFileAHook,
		&hFindNextFileA,
		"FindNextFileA"
	);

	InstallHook(
		GetProcAddress(kernel32, "FindFirstFileW"),
		myFindFirstFileWHook,
		&hFindFirstFileW,
		"FindFirstFileW"
	);

	InstallHook(
		GetProcAddress(kernel32, "FindNextFileW"),
		myFindNextFileWHook,
		&hFindNextFileW,
		"FindNextFileW"
	);

	InstallHook(
		GetProcAddress(kernel32, "CreateFileA"),
		myCreateFileAHook,
		&hCreateFileA,
		"CreateFileA"
	);

	InstallHook(
		GetProcAddress(kernel32, "CreateFileW"),
		myCreateFileWHook,
		&hCreateFileW,
		"CreateFileW"
	);

	InstallHook(
		GetProcAddress(kernel32, "ReadFile"),
		myReadFileHook,
		&hReadFile,
		"ReadFile"
	);

	cout << "[*] File system deception hooks are active." << endl;
}