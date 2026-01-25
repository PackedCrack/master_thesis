#include "common.h"

// Win32
#include <shlobj.h>
// std
#include <assert.h>
#include <stdio.h>


static LPSTR format_message(DWORD code)
{
	LPSTR pMsg = NULL;	// This is correct because FORMAT_MESSAGE_ALLOCATE_BUFFER is used.
	DWORD length = FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		code,
		0,
		&pMsg,
		0,
		NULL);

	if (pMsg == NULL || length == 0)
	{
		fprintf(stderr, "Failed to format Win32 error message.\n");
		return "Failed to obtain error message from Windows.\n";
	}

	return pMsg;
}
static void desktop_filepath(wchar_t* pOut, size_t outSize)
{
	PWSTR pFilepath = NULL;
	HRESULT r = SHGetKnownFolderPath(&FOLDERID_Desktop,
									 0,
									 NULL,
									 &pFilepath);
	if (SUCCEEDED(r))
	{
		size_t len = wcslen(pFilepath);
		assert(len * sizeof(WCHAR) < outSize);

		memcpy(pOut, pFilepath, len * sizeof(WCHAR));
		pOut[len] = L'\0';

		CoTaskMemFree(pFilepath);
	}
	else
	{
		printf("Failed to obtain Desktop filepath.");
		assert(FALSE);
	}
}
static size_t generate_filename(const wchar_t* basename, wchar_t* pOut, size_t outSize)
{
	assert(basename != NULL);
	assert(pOut != NULL);

	size_t nameLen = wcslen(basename);
	assert(nameLen < outSize);
	memcpy(pOut, basename, nameLen * sizeof(WCHAR));

	WCHAR suffix[64] = { 0 };
	swprintf(suffix, ARRAYSIZE(suffix), L"0x%X.txt", rand());

	size_t suffixLen = wcslen(suffix);
	assert(nameLen + suffixLen + 1 < outSize);

	memcpy(pOut + nameLen, suffix, suffixLen * sizeof(WCHAR));
	pOut[nameLen + suffixLen] = L'\0';

	return wcslen(pOut);
}
static size_t get_log_filepath(LPWSTR basename, LPWSTR pOut, size_t outSize)
{
	assert(basename != NULL);
	assert(pOut != NULL);

	WCHAR logPath[512] = { 0 };
	desktop_filepath(logPath, sizeof(logPath));

	WCHAR filename[128] = { 0 };
	size_t filenameLen = generate_filename(basename, filename, sizeof(filename));

	LPWSTR folder = L"\\output\\";

	size_t outMaxLen = outSize / sizeof(WCHAR);
	size_t logpathSize = wcslen(logPath);
	size_t filenameSize = filenameLen;
	size_t folderSize = wcslen(folder);
	assert(logpathSize + filenameSize + folderSize < outMaxLen);

	wcscat_s(pOut, outMaxLen, logPath);
	wcscat_s(pOut, outMaxLen, folder);
	CreateDirectoryW(pOut, NULL);

	wcscat_s(pOut, outMaxLen, filename);

	return wcslen(pOut);
}
//
//
void print_win32_err(const char* func)
{
	DWORD err = GetLastError();
	if (err == FALSE)
	{
		printf("No reported Win32 error.\n");
	}
	else
	{
		LPSTR pMsg = format_message(err);
		fprintf(stderr, "%s failed with code: %i.\n%s\n", func, err, pMsg);
	}
}
HANDLE open_log_file(LPWSTR basename)
{
	WCHAR logPath[1024] = { 0 };
	size_t filenameLen = get_log_filepath(basename, logPath, sizeof(logPath));

	HANDLE hFile = CreateFileW(logPath,
							   GENERIC_WRITE,
							   FILE_SHARE_READ,
							   NULL,
							   CREATE_ALWAYS,
							   FILE_ATTRIBUTE_NORMAL,
							   NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		PRINT_WIN32_ERROR(CreateFileW);
		assert(FALSE);
		return INVALID_HANDLE_VALUE;
	}
	else
	{
		return hFile;
	}
}