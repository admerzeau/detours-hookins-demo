#include <windows.h>
#include <detours.h>
#include <iostream>
#include <vector>

static LONG dwSlept = 0;
bool APPEND_DATA_ON_WRITES = false;
bool SHOW_MESSAGE_BOX_ON_HOOK = true;
const char* CFILENAME = "test";
const wchar_t* WCFILENAME = L"test";

/*Get the pointer to all hooked functions*/
static BOOL(WINAPI* _WriteFile)(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped) = WriteFile;
static HANDLE(WINAPI* _CreateFileA)(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile) = CreateFileA;
static BOOL(WINAPI* _DeleteFileA)(LPCSTR lpFileName) = DeleteFileA;
static BOOL(WINAPI* _DeleteFileW)(LPCWSTR lpFileName) = DeleteFileW;
static VOID(WINAPI* _Sleep)(DWORD dwMilliseconds) = Sleep;


BOOL WINAPI HookedWriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped)
{
    std::cout << "Hooked function WriteFile\n";
    if(SHOW_MESSAGE_BOX_ON_HOOK)
        MessageBoxA(0, "WriteFile was called", "Info", 0);

    if (APPEND_DATA_ON_WRITES)
    {
        const char* fileData = reinterpret_cast<const char*>(lpBuffer);
        const char* hack = "\n Adding something to the file";
        std::string str(fileData);
        str.append(hack);
        return _WriteFile(hFile, str.data(), str.size(), lpNumberOfBytesWritten, lpOverlapped);
    }
    else
    {
        return _WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
    }
}

HANDLE WINAPI HookedCreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,HANDLE hTemplateFile)
{
    std::cout << "Hooked function CreateFileA\n";
    if (SHOW_MESSAGE_BOX_ON_HOOK)
        MessageBoxA(0, "CreateFileA was called", "Info", 0);

    return _CreateFileA(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

BOOL WINAPI HookedDeleteFileA(LPCSTR lpFileName)
{
    std::cout << "Hooked function DeleteFileA\n";
    if (SHOW_MESSAGE_BOX_ON_HOOK)
        MessageBoxA(0, "DeleteFileA was called", "Info", 0);

    if (strstr(lpFileName, CFILENAME))
    {
        SetLastError(ERROR_ACCESS_DENIED);
        MessageBoxA(0, "This file cannot be deleted. Hooked function don't allow it.", "Error", 0);
        return false;
    }
    else
    {
       return _DeleteFileA(lpFileName);
    }
}

BOOL WINAPI HookedDeleteFileW(LPCWSTR lpFileName)
{
    std::cout << "Hooked function DeleteFileW\n";
    if (SHOW_MESSAGE_BOX_ON_HOOK)
        MessageBoxA(0, "DeleteFileW was called", "Info", 0);

    if (wcsstr(lpFileName, WCFILENAME))
    {
        SetLastError(ERROR_ACCESS_DENIED);
        MessageBoxA(0, "This file cannot be deleted. Hooked function don't allow it.", "Error", 0);
        return false;
    }
    else
    {
        return _DeleteFileW(lpFileName);
    }
}

VOID WINAPI HookedSleep(DWORD dwMilliseconds)
{
    std::cout << "Hooked function Sleep\n";
    _Sleep(dwMilliseconds);
}

// DllMain function attaches and detaches the functions detours
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID reserved)
{
    if (DetourIsHelperProcess()) {
        return TRUE;
    }

    if (dwReason == DLL_PROCESS_ATTACH) {
        DetourRestoreAfterWith();

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(&(PVOID&)_Sleep, HookedSleep);
        DetourAttach(&(PVOID&)_WriteFile, HookedWriteFile);
        DetourAttach(&(PVOID&)_CreateFileA, HookedCreateFileA);
        DetourAttach(&(PVOID&)_DeleteFileA, HookedDeleteFileA);
        DetourAttach(&(PVOID&)_DeleteFileW, HookedDeleteFileW);
        DetourTransactionCommit();
    }
    else if (dwReason == DLL_PROCESS_DETACH) {
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourDetach(&(PVOID&)_Sleep, HookedSleep);
        DetourDetach(&(PVOID&)_WriteFile, HookedWriteFile);
        DetourDetach(&(PVOID&)_CreateFileA, HookedCreateFileA);
        DetourDetach(&(PVOID&)_DeleteFileA, HookedDeleteFileA);
        DetourDetach(&(PVOID&)_DeleteFileW, HookedDeleteFileW);
        DetourTransactionCommit();
    }
    return TRUE;
}