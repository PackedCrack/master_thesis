#include "T1005.h"

#include "../misc/common.h"
#include "../runtime_linking.h"
#include "../misc/vector.h"
#include "../misc/wstr.h"

#include <assert.h>
#include <stdio.h>

#define WIN_LEAN_AND_MEAN
#include <Windows.h>
#include <ShlObj_core.h>

// Kernel32.dll
#define CLOSE_HANDLE 0
#define K32_ENUM_PROCESSES 1
#define OPEN_PROCESS 2
#define K32_GET_MODULE_BASE_NAME_W 3
#define GET_LAST_ERROR 4
static const char* kernel32Procedures[5] = { "CloseHandle", "K32EnumProcesses", "OpenProcess", "K32GetModuleBaseNameW",
												"GetLastError" };
// Shell32.dll
#define SH_GET_KNOWN_FOLDER_PATH 0
static const char* shell32Procedures[1] = { "SHGetKnownFolderPath" };
// Ole32.dll
#define CO_TASK_MEM_FREE 0
static const char* ole32Procedures[1] = { "CoTaskMemFree" };


WideString resolve_filepath(ProcedureList* pShell32, ProcedureList* pOle32, REFKNOWNFOLDERID knownFolder)
{
	PWSTR pFilepath = NULL;
	FARPROC PFN_SHGetKnownFolderPath = *VECTOR_AT(pShell32->procedures, FARPROC, SH_GET_KNOWN_FOLDER_PATH);
	HRESULT r = PFN_SHGetKnownFolderPath(knownFolder,
									 0,
									 NULL,
									 &pFilepath);
	if (!SUCCEEDED(r))
	{
		printf("Failed to resolve filepath. Error: 0x%lX.\n", r);
		return (WideString) { 0 };
	}
	
	WideString s = WSTRING_CREATE_FROM_LPCWSTR(pFilepath);

	FARPROC PFN_CoTaskMemFree = *VECTOR_AT(pOle32->procedures, FARPROC, CO_TASK_MEM_FREE);
	PFN_CoTaskMemFree(pFilepath);

	return s;
}
static WideString get_acc_picture_path(ProcedureList* pShell32, ProcedureList* pOle32)
{
	return resolve_filepath(pShell32, pOle32, &FOLDERID_AccountPictures);
}
static WideString get_camera_roll_path(ProcedureList* pShell32, ProcedureList* pOle32)
{
	return resolve_filepath(pShell32, pOle32, &FOLDERID_CameraRoll);
}
static WideString get_desktop_path(ProcedureList* pShell32, ProcedureList* pOle32)
{
	return resolve_filepath(pShell32, pOle32, &FOLDERID_Desktop);
}
static WideString get_documents_path(ProcedureList* pShell32, ProcedureList* pOle32)
{
	return resolve_filepath(pShell32, pOle32, &FOLDERID_Documents);
}
static WideString get_downloads_path(ProcedureList* pShell32, ProcedureList* pOle32)
{
	return resolve_filepath(pShell32, pOle32, &FOLDERID_Downloads);
}
static WideString get_photo_album_path(ProcedureList* pShell32, ProcedureList* pOle32)
{
	return resolve_filepath(pShell32, pOle32, &FOLDERID_PhotoAlbums);
}
static WideString get_pictures_path(ProcedureList* pShell32, ProcedureList* pOle32)
{
	return resolve_filepath(pShell32, pOle32, &FOLDERID_Pictures);
}
static Vector create_root_dirs(ProcedureList* pShell32, ProcedureList* pOle32)
{
	Vector roots = VECTOR_CREATE(WideString, 0);

	WideString accPic = get_acc_picture_path(pShell32, pOle32);
	if (WSTRING_SIZE(accPic) > 0)
	{
		VECTOR_PUSH_BACK(roots, WideString, accPic);
	}
	WideString camRoll = get_camera_roll_path(pShell32, pOle32);
	if (WSTRING_SIZE(camRoll) > 0)
	{
		VECTOR_PUSH_BACK(roots, WideString, camRoll);
	}
	WideString desktop = get_desktop_path(pShell32, pOle32);
	if (WSTRING_SIZE(desktop) > 0)
	{
		VECTOR_PUSH_BACK(roots, WideString, desktop);
	}
	WideString documents = get_documents_path(pShell32, pOle32);
	if (WSTRING_SIZE(documents) > 0)
	{
		VECTOR_PUSH_BACK(roots, WideString, documents);
	}
	WideString downloads = get_downloads_path(pShell32, pOle32);
	if (WSTRING_SIZE(downloads) > 0)
	{
		VECTOR_PUSH_BACK(roots, WideString, downloads);
	}
	WideString photoAlbum = get_photo_album_path(pShell32, pOle32);
	if (WSTRING_SIZE(photoAlbum) > 0)
	{
		VECTOR_PUSH_BACK(roots, WideString, photoAlbum);
	}
	WideString pictures = get_pictures_path(pShell32, pOle32);
	if (WSTRING_SIZE(pictures) > 0)
	{
		VECTOR_PUSH_BACK(roots, WideString, pictures);
	}

	return roots;
}
static void destroy_root_dirs(Vector* pRoots)
{
	while (!VECTOR_EMPTY(*pRoots))
	{
		WideString* pDir = VECTOR_BACK(*pRoots, WideString);
		WSTRING_DESTROY(*pDir);

		VECTOR_POP_BACK(*pRoots);
	}

	VECTOR_DESTROY(*pRoots);
}
//
//
void execute_t1005()
{
	ProcedureList kernel32 = procedure_list_create("kernel32.dll", kernel32Procedures, ARRAYSIZE(kernel32Procedures));
	ProcedureList shell32 = procedure_list_create("shell32.dll", shell32Procedures, ARRAYSIZE(shell32Procedures));
	ProcedureList ole32 = procedure_list_create("ole32.dll", ole32Procedures, ARRAYSIZE(ole32Procedures));
	
	Vector roots = create_root_dirs(&shell32, &ole32);

	for (size_t i = 0; i < VECTOR_SIZE(roots); ++i)
	{
		WideString* pDir = VECTOR_AT(roots, WideString, i);
		printf("Directory: %ls\n", WSTRING_C_STR(*pDir));
	}

	destroy_root_dirs(&roots);

	//FARPROC PFN_CloseHandle = *VECTOR_AT(pKernel32->procedures, FARPROC, CLOSE_HANDLE);
	
	procedure_list_destroy(&ole32);
	procedure_list_destroy(&shell32);
	procedure_list_destroy(&kernel32);
}