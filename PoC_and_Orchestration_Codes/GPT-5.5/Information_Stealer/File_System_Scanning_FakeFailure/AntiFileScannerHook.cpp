#include "pch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <string>
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

const string protectedDirectoryA =
"C:\\Users\\User\\Documents\\MalwareAnalysis\\ClipboardLogger\\Debug";

const wstring protectedDirectoryW =
L"C:\\Users\\User\\Documents\\MalwareAnalysis\\ClipboardLogger\\Debug";

bool StartsWithIgnoreCaseA(const string& value, const string& prefix)
{
	if (prefix.size() > value.size())
		return false;

	return _strnicmp(value.c_str(), prefix.c_str(), prefix.size()) == 0;
}

bool StartsWithIgnoreCaseW(const wstring& value, const wstring& prefix)
{
	if (prefix.size() > value.size())
		return false;

	return _wcsnicmp(value.c_str(), prefix.c_str(), prefix.size()) == 0;
}

bool IsProtectedSearchPathA(LPCSTR lpFileName)
{
	if (lpFileName == nullptr)
		return false;

	string path = lpFileName;
	return StartsWithIgnoreCaseA(path, protectedDirectoryA);
}

bool IsProtectedSearchPathW(LPCWSTR lpFileName)
{
	if (lpFileName == nullptr)
		return false;

	wstring path = lpFileName;
	return StartsWithIgnoreCaseW(path, protectedDirectoryW);
}

HANDLE WINAPI myFindFirstFileAHook(
	LPCSTR lpFileName,
	LPWIN32_FIND_DATAA lpFindFileData)
{
	cout << "[Hook] FindFirstFileA intercepted: "
		<< (lpFileName ? lpFileName : "NULL") << endl;

	if (IsProtectedSearchPathA(lpFileName))
	{
		cout << "[FakeFailure] File search operation interrupted." << endl;

		SetLastError(ERROR_OPERATION_ABORTED);
		return INVALID_HANDLE_VALUE;
	}

	return FindFirstFileA(lpFileName, lpFindFileData);
}

BOOL WINAPI myFindNextFileAHook(
	HANDLE hFindFile,
	LPWIN32_FIND_DATAA lpFindFileData)
{
	cout << "[Hook] FindNextFileA intercepted." << endl;

	SetLastError(ERROR_OPERATION_ABORTED);
	cout << "[FakeFailure] File search operation interrupted." << endl;

	return FALSE;
}

HANDLE WINAPI myFindFirstFileWHook(
	LPCWSTR lpFileName,
	LPWIN32_FIND_DATAW lpFindFileData)
{
	wcout << L"[Hook] FindFirstFileW intercepted: "
		<< (lpFileName ? lpFileName : L"NULL") << endl;

	if (IsProtectedSearchPathW(lpFileName))
	{
		wcout << L"[FakeFailure] File search operation interrupted." << endl;

		SetLastError(ERROR_OPERATION_ABORTED);
		return INVALID_HANDLE_VALUE;
	}

	return FindFirstFileW(lpFileName, lpFindFileData);
}

BOOL WINAPI myFindNextFileWHook(
	HANDLE hFindFile,
	LPWIN32_FIND_DATAW lpFindFileData)
{
	wcout << L"[Hook] FindNextFileW intercepted." << endl;

	SetLastError(ERROR_OPERATION_ABORTED);
	wcout << L"[FakeFailure] File search operation interrupted." << endl;

	return FALSE;
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
	cout << "[Hook] CreateFileA allowed: "
		<< (lpFileName ? lpFileName : "NULL") << endl;

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
	wcout << L"[Hook] CreateFileW allowed: "
		<< (lpFileName ? lpFileName : L"NULL") << endl;

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

	if (result && lpNumberOfBytesRead)
	{
		cout << "[Hook] ReadFile allowed. Bytes read: "
			<< *lpNumberOfBytesRead << endl;
	}

	return result;
}

void InstallHook(
	HMODULE module,
	LPCSTR apiName,
	PVOID hookFunction,
	HOOK_TRACE_INFO* hookInfo)
{
	FARPROC target = GetProcAddress(module, apiName);

	if (target == nullptr)
	{
		cout << "[-] Failed to resolve API: " << apiName << endl;
		return;
	}

	NTSTATUS result = LhInstallHook(
		target,
		hookFunction,
		nullptr,
		hookInfo
	);

	if (FAILED(result))
	{
		wcout << L"[-] Failed to install hook for "
			<< apiName << L": "
			<< RtlGetLastErrorString() << endl;
		return;
	}

	cout << "[+] Hook installed: " << apiName << endl;

	ULONG ACLEntries[1] = { 0 };
	LhSetExclusiveACL(ACLEntries, 1, hookInfo);
}

extern "C" void __declspec(dllexport) __stdcall NativeInjectionEntryPoint(
	REMOTE_ENTRY_INFO * inRemoteInfo)
{
	cout << "[*] File scanning FakeFailure DLL injected." << endl;

	HMODULE kernel32 = GetModuleHandleA("kernel32.dll");

	InstallHook(kernel32, "FindFirstFileA", myFindFirstFileAHook, &hFindFirstFileA);
	InstallHook(kernel32, "FindNextFileA", myFindNextFileAHook, &hFindNextFileA);

	InstallHook(kernel32, "FindFirstFileW", myFindFirstFileWHook, &hFindFirstFileW);
	InstallHook(kernel32, "FindNextFileW", myFindNextFileWHook, &hFindNextFileW);

	InstallHook(kernel32, "CreateFileA", myCreateFileAHook, &hCreateFileA);
	InstallHook(kernel32, "CreateFileW", myCreateFileWHook, &hCreateFileW);
	InstallHook(kernel32, "ReadFile", myReadFileHook, &hReadFile);

	cout << "[*] FakeFailure hooks are active." << endl;
}