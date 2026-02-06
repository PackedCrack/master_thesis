#include "T1070.004.h"

#include "../misc/common.h"
#include "../misc/function_pointers.h"
#include "../misc/wstr.h"
#include "../runtime_linking.h"

#include <assert.h>
#include <stdio.h>

#define WIN_LEAN_AND_MEAN
#include <Windows.h>


// Kernel32.dll
#define CLOSE_HANDLE 0
#define REMOVE_DIRECTORY_W 1
#define GET_FILE_ATTRIBUTES_W 2
#define DELETE_FILE_A 3
#define GET_LAST_ERROR 4
static const char* kernel32Procedures[5] = { "CloseHandle", "RemoveDirectoryW", "GetFileAttributesW",
												"DeleteFileA", "GetLastError" };
// Shlwapi.dll
#define PATH_IS_DIRECTORY_W 0
#define PATH_FILE_EXISTS_A 1
static const char* shlwapiProcedures[2] = { "PathIsDirectoryW", "PathFileExistsA" };


static WideString log_directory()
{
	WCHAR desktop[MAX_PATH] = { 0 };
	desktop_filepath(desktop, ARRAYSIZE(desktop));
	WCHAR tmp[MAX_PATH * 2] = { 0 };
	swprintf(tmp, ARRAYSIZE(tmp), L"%ls\\output", desktop);

	return WSTRING_CREATE_FROM_LPCWSTR(tmp);
}
static BOOL exists(ProcedureList* pShlwapi, WideString* pLogDir)
{
	PFN_PathIsDirectoryW path_is_directory_w = *VECTOR_AT(pShlwapi->procedures, PFN_PathIsDirectoryW, PATH_IS_DIRECTORY_W);
	return path_is_directory_w(WSTRING_C_STR(*pLogDir));
}
static void erase_logs(ProcedureList* pKernel32, ProcedureList* pShlwapi)
{
	WideString logDirectory = log_directory();
	if (exists(pShlwapi , &logDirectory))
	{
		PFN_RemoveDirectoryW remove_directory_w = *VECTOR_AT(pKernel32->procedures, PFN_RemoveDirectoryW, REMOVE_DIRECTORY_W);
		if (!remove_directory_w(WSTRING_C_STR(logDirectory)))
		{
			PRINT_WIN32_ERROR(RemoveDirectoryW);
			assert(FALSE);
		}
	}

	WSTRING_DESTROY(logDirectory);
}
static BOOL file_exists(ProcedureList* pShlwapi, const char* path)
{
	PFN_PathFileExistsA path_file_exists_a = *VECTOR_AT(pShlwapi->procedures, PFN_PathFileExistsA, PATH_FILE_EXISTS_A);
	if (!path_file_exists_a(path))
	{
		PRINT_WIN32_ERROR(PathFileExistsA);
		return FALSE;
	}

	return TRUE;
}
static BOOL is_inside_quotes(const char* str, size_t len)
{
	if (str[0] != '"' || str[len - 1] != '"')
	{
		return FALSE;
	}

	return TRUE;
}
static void self_delete(ProcedureList* pKernel32, ProcedureList* pShlwapi, char** argv)
{
	const char* self = argv[0];
	if (!file_exists(pShlwapi, self))
	{
		return;
	}

	PFN_DeleteFileA delete_file_a = *VECTOR_AT(pKernel32->procedures, PFN_DeleteFileA, DELETE_FILE_A);
	if (!delete_file_a(self))
	{

		PFN_GetLastError get_last_error = *VECTOR_AT(pKernel32->procedures, PFN_GetLastError, GET_LAST_ERROR);
		DWORD err = get_last_error();
		if (err != ERROR_ACCESS_DENIED)
		{
			PRINT_WIN32_ERROR(DeleteFileA);
			assert(FALSE);
		}
	}
}
//
//
void execute_t1070_004(char** argv)
{
	ProcedureList kernel32 = procedure_list_create("kernel32.dll", kernel32Procedures, ARRAYSIZE(kernel32Procedures));
	ProcedureList shlwapi = procedure_list_create("shlwapi.dll", shlwapiProcedures, ARRAYSIZE(shlwapiProcedures));

	erase_logs(&kernel32, &shlwapi);
	self_delete(&kernel32, &shlwapi, argv);

	procedure_list_destroy(&shlwapi);
	procedure_list_destroy(&kernel32);
}