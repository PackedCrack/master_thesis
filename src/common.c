#include "common.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h>


static LPSTR format_message(DWORD code)
{
	LPSTR pMsg = NULL;
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