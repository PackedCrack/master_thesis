#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <ShlObj.h>
#include <imagehlp.h>
#include <mscat.h>


// Kernel32.dll
typedef BOOL(__stdcall* PFN_CreateDirectoryW)(LPCWSTR, LPSECURITY_ATTRIBUTES);
typedef HANDLE(__stdcall* PFN_CreateFileW)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
typedef DWORD(__stdcall* PFN_FormatMessageA)(DWORD, LPCVOID, DWORD, DWORD, LPSTR, DWORD, va_list*);
typedef DWORD(__stdcall* PFN_GetLastError)(void);
typedef BOOL(__stdcall* PFN_WriteFile)(HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED);
typedef HANDLE(__stdcall* PFN_FindFirstFileW)(LPCWSTR, LPWIN32_FIND_DATAW);
typedef BOOL(__stdcall* PFN_FindNextFileW)(HANDLE, LPWIN32_FIND_DATAW);
typedef BOOL(__stdcall* PFN_FindClose)(HANDLE);
typedef BOOL(__stdcall* PFN_CloseHandle)(HANDLE);
typedef BOOL(__stdcall* PFN_K32EnumProcesses)(DWORD*, DWORD, LPDWORD);
typedef HANDLE(__stdcall* PFN_OpenProcess)(DWORD, BOOL, DWORD);
typedef DWORD(__stdcall* PFN_K32GetModuleBaseNameW)(HANDLE, HMODULE, LPWSTR, DWORD);
typedef BOOL(__stdcall* PFN_CreateProcessW)(LPCWSTR, LPWSTR, LPSECURITY_ATTRIBUTES, LPSECURITY_ATTRIBUTES, BOOL, DWORD, LPVOID, LPCWSTR, LPSTARTUPINFOW, LPPROCESS_INFORMATION);
typedef DWORD(__stdcall* PFN_WaitForSingleObject)(HANDLE, DWORD);
typedef UINT(__stdcall* PFN_WinExec)(LPCSTR, UINT);
typedef BOOL(__stdcall* PFN_RemoveDirectoryW)(LPCWSTR);
typedef DWORD(__stdcall* PFN_GetFileAttributesW)(LPCWSTR);
typedef BOOL(__stdcall* PFN_DeleteFileA)(LPCSTR);
typedef BOOL(__stdcall* PFN_DeleteFileW)(LPCWSTR);
typedef BOOL(__stdcall* PFN_GetComputerNameExW)(COMPUTER_NAME_FORMAT, LPWSTR, LPDWORD);
typedef void(__stdcall* PFN_GetNativeSystemInfo)(LPSYSTEM_INFO);
typedef BOOL(__stdcall* PFN_GlobalMemoryStatusEx)(LPMEMORYSTATUSEX);
typedef BOOL(__stdcall* PFN_FindVolumeClose)(HANDLE);
typedef HANDLE(__stdcall* PFN_FindFirstVolumeW)(LPWSTR, DWORD);
typedef BOOL(__stdcall* PFN_FindNextVolumeW)(HANDLE, LPWSTR, DWORD);
typedef int(__stdcall* PFN_MultiByteToWideChar)(UINT, DWORD, LPCCH, int, LPWSTR, int);
typedef BOOL(__stdcall* PFN_GetVolumePathNamesForVolumeNameW)(LPCWSTR, LPWCH, DWORD, PDWORD);
// Shlwapi.dll
typedef BOOL(__stdcall* PFN_PathIsDirectoryW)(LPCWSTR);
typedef BOOL(__stdcall* PFN_PathFileExistsA)(LPCSTR);
// Imagehlp.dll
typedef BOOL(__stdcall* PFN_ImageEnumerateCertificates)(HANDLE, WORD, PDWORD, PDWORD, DWORD);
// Wintrust.dll
typedef BOOL(__stdcall* PFN_CryptCATAdminCalcHashFromFileHandle2)(HCATADMIN, HANDLE, DWORD*, BYTE*, DWORD);
typedef HCATINFO(__stdcall* PFN_CryptCATAdminEnumCatalogFromHash)(HCATADMIN, BYTE*, DWORD, DWORD, HCATINFO*);
typedef BOOL(__stdcall* PFN_CryptCATAdminAcquireContext2)(HCATADMIN*, const GUID*, PCWSTR, PCERT_STRONG_SIGN_PARA, DWORD);
typedef BOOL(__stdcall* PFN_CryptCATAdminReleaseCatalogContext)(HCATADMIN, HCATINFO, DWORD);
typedef BOOL(__stdcall* PFN_CryptCATAdminReleaseContext)(HCATADMIN, DWORD);
// Advapi32.dll
typedef BOOL(__stdcall* PFN_OpenProcessToken)(HANDLE, DWORD, PHANDLE);
typedef BOOL(__stdcall* PFN_GetTokenInformation)(HANDLE, TOKEN_INFORMATION_CLASS, LPVOID, DWORD, PDWORD);
typedef BOOL(__stdcall* PFN_LookupPrivilegeNameW)(LPCWSTR, PLUID, LPWSTR, LPDWORD);
typedef LSTATUS(__stdcall* PFN_RegGetValueW)(HKEY, LPCWSTR, LPCWSTR, DWORD, LPDWORD, PVOID, LPDWORD);
typedef LSTATUS(__stdcall* PFN_RegCreateKeyExW)(HKEY, LPCWSTR, DWORD, LPWSTR, DWORD, REGSAM, const LPSECURITY_ATTRIBUTES, PHKEY, LPDWORD);
typedef LSTATUS(__stdcall* PFN_RegSetValueExA)(HKEY, LPCSTR, DWORD, DWORD, const BYTE*, DWORD);
typedef LSTATUS(__stdcall* PFN_RegCloseKey)(HKEY);
// Crypt32.dll
typedef BOOL(__stdcall* PFN_CryptBinaryToStringA)(const BYTE*, DWORD, DWORD, LPSTR, DWORD*);
// Shell32.dll
typedef HRESULT(__stdcall* PFN_SHGetKnownFolderPath)(REFKNOWNFOLDERID, DWORD, HANDLE, PWSTR*);
typedef HINSTANCE(__stdcall* PFN_ShellExecuteW)(HWND, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, INT);
// Ole32.dll
typedef void(__stdcall* PFN_CoTaskMemFree)(LPVOID);
typedef HRESULT(__stdcall* PFN_CoInitializeEx)(LPVOID, DWORD);
typedef HRESULT(__stdcall* PFN_CoCreateInstance)(REFCLSID, LPUNKNOWN, DWORD, REFIID, LPVOID*);
typedef void(__stdcall* PFN_CoUninitialize)(void);
