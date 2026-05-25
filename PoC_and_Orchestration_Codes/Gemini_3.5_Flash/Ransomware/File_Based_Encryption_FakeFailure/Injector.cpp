#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <easyhook.h>

#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "EasyHook32.lib") // x64 build
// #pragma comment(lib, "EasyHook64.lib") // x64 build

static void PrintLastError(const wchar_t* prefix) {
    DWORD err = GetLastError();
    if (!err) { std::wcerr << prefix << L": (no error)\n"; return; }
    wchar_t* msg = nullptr;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, err, 0, (LPWSTR)&msg, 0, nullptr);
    if (msg) { std::wcerr << prefix << L": " << msg; LocalFree(msg); }
}

static std::wstring DirOfExe() {
    wchar_t buf[MAX_PATH]{};
    DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    if (!n || n >= MAX_PATH) return L"";
    // strip filename
    wchar_t* slash = wcsrchr(buf, L'\\');
    if (slash) *slash = L'\0';
    return buf;
}

static std::wstring JoinPath(const std::wstring& a, const std::wstring& b) {
    if (a.empty()) return b;
    if (!a.empty() && (a.back() == L'\\' || a.back() == L'/')) return a + b;
    return a + L'\\' + b;
}

static bool FileExistsW(const std::wstring& p) {
    DWORD attr = GetFileAttributesW(p.c_str());
    return (attr != INVALID_FILE_ATTRIBUTES) && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

int wmain() {
    // Your target exe and args
    const std::wstring targetExe = LR"(C:\Users\User\Documents\MalwareAnalysis\ClipboardLogger\Debug\FileEncryptionPOC.exe)";
    //const std::wstring targetExe = LR"(C:\Users\User\Documents\MalwareAnalysis\KeyLogger-main\Debug\bitblt_infiniteblue.exe)";
    //const std::wstring targetExe = LR"(C:\Users\User\Documents\MalwareAnalysis\myhook-master\Debug\myhook.exe)";
    //const std::wstring targetArgs = L" test.bmp test2.bmp output.bmp";

    // Build absolute path to the DLL relative to the injector EXE folder
    // Adjust the relative layout as needed; here DLL lives next to the injector in ..\x64\Debug\

    const std::wstring injectorDir = DirOfExe();                  // e.g., ...\ClipboardLogger\x64\Debug
    const std::wstring dllRel = L"..\\Debug\\AntiFileEncryptionHook.dll";
    const std::wstring dllMaybeRel = JoinPath(injectorDir, dllRel);

    // Canonicalize to absolute full path (optional but nice for printing)
    wchar_t dllFull[MAX_PATH]{};
    if (!GetFullPathNameW(dllMaybeRel.c_str(), MAX_PATH, dllFull, nullptr)) {
        std::wcerr << L"[!] GetFullPathNameW failed for DLL path.\n";
        return 1;
    }

    std::wcout << L"[*] Resolved 64-bit DLL path: " << dllFull << std::endl;

    if (!FileExistsW(dllFull)) {
        std::wcerr << L"[!] DLL file does not exist at that path. Check your build output location.\n";
        return 2;
    }

    // Create target suspended
    //std::wstring cmd = L"\"" + targetExe + L"\"" + targetArgs;
    std::wstring cmd = L"\"" + targetExe;
    std::vector<wchar_t> cmdBuf(cmd.begin(), cmd.end()); cmdBuf.push_back(0);

    STARTUPINFOW si{}; si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    if (!CreateProcessW(nullptr, cmdBuf.data(), nullptr, nullptr, FALSE,
        CREATE_SUSPENDED | CREATE_UNICODE_ENVIRONMENT,
        nullptr, nullptr, &si, &pi)) {
        PrintLastError(L"[!] CreateProcessW failed");
        return 3;
    }
    /*if (!CreateProcessW(nullptr, cmdBuf.data(), nullptr, nullptr, FALSE,
        CREATE_SUSPENDED | CREATE_UNICODE_ENVIRONMENT,
        nullptr, nullptr, &si, &pi)) {
        PrintLastError(L"[!] CreateProcessW failed");
        return 3;
    }*/
    std::wcout << L"[*] Process created suspended. PID=" << pi.dwProcessId
        << L", TID=" << pi.dwThreadId << std::endl;

    // Inject x64 DLL (pass NULL for 32-bit)
    NTSTATUS nt = RhInjectLibrary(
        pi.dwProcessId,
        0,
        EASYHOOK_INJECT_DEFAULT,
        (WCHAR*)dllFull,                    // 32-bit DLL: none
        NULL,         // 64-bit DLL: absolute path we resolved
        nullptr, 0
    );

    if (nt != 0) {
        std::wcerr << L"[!] RhInjectLibrary failed. NTSTATUS: 0x"
            << std::hex << nt << std::dec << std::endl;
        try { std::wcout << RtlGetLastErrorString() << std::endl; }
        catch (...) {}
        std::wcerr << L"[*] Terminating the suspended target process (injection failed).\n";
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hThread); CloseHandle(pi.hProcess);
        return 4;
    }

    std::wcout << L"[+] DLL injected successfully.\n";
    DWORD prev = ResumeThread(pi.hThread);
    if (prev == (DWORD)-1) { PrintLastError(L"[!] ResumeThread failed"); }
    CloseHandle(pi.hThread); CloseHandle(pi.hProcess);
    return 0;
}


/*
#include <iostream>
#include <windows.h>
#include <tchar.h>
#include <string>
#include <cstring>
#include <easyhook.h>
#include <vector>

int wmain()
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Path to the target executable
    LPCWSTR exePath = L"C:\\Users\\User\\Documents\\MalwareAnalysis\\ClipboardLogger\\Debug\\ClipboardLoggerPOC.exe";

    // Command line including arguments
    // WCHAR commandLine[] = L"\"C:\\Users\\User\\Documents\\MalwareAnalysis\\Ransomware-master\\build\\windows\\release\\x64\\Winsomware.exe\" scaesar \"12as\" encrypt print test.txt";

    // std::wstring cmd = L"\"C:\\Users\\User\\Documents\\MalwareAnalysis\\Ransomware-master\\build\\windows\\release\\x64\\Winsomware.exe\" scaesar \"12as\" encrypt print test.txt";
    // CreateProcess requires a writable buffer for lpCommandLine.
    // std::vector<wchar_t> cmdBuf(cmd.begin(), cmd.end());
    // cmdBuf.push_back(L'\0'); // null-terminate

    LPCWSTR workingDir = L"C:\\Users\\User\\Documents\\MalwareAnalysis\\ClipboardLogger\\Debug";

    // Create the process in suspended mode
    if (!CreateProcess(
        exePath,         // Application name
        NULL,     // Command line
        NULL,            // Process handle not inheritable
        NULL,            // Thread handle not inheritable
        FALSE,           // Set handle inheritance to FALSE
        CREATE_SUSPENDED,// Creation flags
        NULL,            // Use parent's environment block
        workingDir,            // Use parent's starting directory
        &si,             // Pointer to STARTUPINFO structure
        &pi)             // Pointer to PROCESS_INFORMATION structure
        )
    {
        std::wcerr << L"CreateProcess failed with error code: " << GetLastError() << std::endl;
        return 1;
    }

    DWORD processId = pi.dwProcessId;
    std::wcout << L"Created suspended process with PID: " << processId << std::endl;

    // Path to the DLL to inject
    LPCWSTR dllToInject = L"..\\Debug\\AntiClipboardLoggerHook.dll";
    std::wcout << L"Attempting to inject: " << dllToInject << std::endl;

    // Optional: Pass custom data to the DLL (can be NULL)
    DWORD dummyData = 0;

    NTSTATUS nt = RhInjectLibrary(
        processId,
        0, // inject into all threads
        EASYHOOK_INJECT_DEFAULT,
        (WCHAR*)dllToInject,          // 32-bit DLL
        NULL,   // 64-bit DLL
        &dummyData,
        sizeof(dummyData)
    );

    if (nt != 0)
    {
        std::wcerr << L"RhInjectLibrary failed with error code = " << nt << std::endl;
        std::wcerr << RtlGetLastErrorString() << std::endl;
        TerminateProcess(pi.hProcess, 1);
        return 1;
    }

    std::wcout << L"Library injected successfully." << std::endl;

    // Resume the main thread of the suspended process
    ResumeThread(pi.hThread);
    std::wcout << L"Process resumed." << std::endl;

    std::wcout << L"Press Enter to exit...";
    std::wstring dummy;
    std::getline(std::wcin, dummy);

    return 0;
}
*/