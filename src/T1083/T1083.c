#include "T1083.h"

#include "../misc/common.h"
#include "../misc/function_pointers.h"
#include "../misc/wstr.h"
#include "../runtime_linking.h"

#include <assert.h>
#include <stdio.h>

#define WIN_LEAN_AND_MEAN
#include <Windows.h>
#include <imagehlp.h>
#include <wintrust.h>
#include <softpub.h>
#include <mscat.h>
#include <mssip.h>


// Kernel32.dll
#define CLOSE_HANDLE 0
#define FIND_VOLUME_CLOSE 1
#define FIND_FIRST_VOLUME_W 2
#define GET_VOLUME_PATH_NAMES_FOR_VOLUME_NAME_W 3
#define FIND_NEXT_VOLUME_W 4
#define FINE_FIRST_FILE_W 5
#define GET_LAST_ERROR 6
#define FIND_CLOSE 7
#define CREATE_FILE_W 8
#define FIND_NEXT_FILE_W 9
static const char* kernel32Procedures[10] = { "CloseHandle", "FindVolumeClose", "FindFirstVolumeW", 
											"GetVolumePathNamesForVolumeNameW", "FindNextVolumeW", "FindFirstFileW",
											"GetLastError", "FindClose", "CreateFileW", "FindNextFileW"};
// imagehlp.dll
#define IMAGE_ENUMERATE_CERTIFICATES 0
static const char* imagehlpProcedures[1] = { "ImageEnumerateCertificates" };

// Wintrust.dll
#define CRYPT_CAT_ADMIN_ACQUIRE_CONTEXT_2 0
#define CRYPT_CAT_ADMIN_CALC_HASH_FROM_FILE_HANDLE_2 1
#define CRYPT_CAT_ADMIN_ENUM_CATALOG_FROM_HASH 2
#define CRYPT_CAT_ADMIN_RELEASE_CATALOG_CONTEXT 3
#define CRYPT_CAT_ADMIN_RELEASE_CONTEXT 4
static const char* wintrustProcedures[5] = { "CryptCATAdminAcquireContext2", "CryptCATAdminCalcHashFromFileHandle2",
												"CryptCATAdminEnumCatalogFromHash", "CryptCATAdminReleaseCatalogContext",
												"CryptCATAdminReleaseContext" };


static DWORD get_hash_size(ProcedureList* pWintrust, HCATADMIN hCatAdmin, HANDLE hFile)
{
	DWORD cbHash = 0;
	PFN_CryptCATAdminCalcHashFromFileHandle2 
		crypt_cat_admin_calc_hash_from_file_handle_2 = *VECTOR_AT(pWintrust->procedures, 
																 PFN_CryptCATAdminCalcHashFromFileHandle2, 
																 CRYPT_CAT_ADMIN_CALC_HASH_FROM_FILE_HANDLE_2);
	if (!crypt_cat_admin_calc_hash_from_file_handle_2(hCatAdmin, hFile, &cbHash, NULL, 0))
	{
		PRINT_WIN32_ERROR(CryptCATAdminCalcHashFromFileHandle2);
		assert(FALSE);
		return 0;
	}

	return cbHash;
}
static Vector get_hash(ProcedureList* pWintrust, HCATADMIN hCatAdmin, HANDLE hFile)
{
	// 3) Compute hash
	DWORD size = get_hash_size(pWintrust, hCatAdmin, hFile);
	if (size > 0)
	{
		Vector hash = VECTOR_CREATE(BYTE, size);
		PFN_CryptCATAdminCalcHashFromFileHandle2
			crypt_cat_admin_calc_hash_from_file_handle_2 = *VECTOR_AT(pWintrust->procedures,
																	  PFN_CryptCATAdminCalcHashFromFileHandle2,
																	  CRYPT_CAT_ADMIN_CALC_HASH_FROM_FILE_HANDLE_2);
		if (crypt_cat_admin_calc_hash_from_file_handle_2(hCatAdmin, hFile, &size, hash.pData, 0))
		{
			return hash;
		}
		else
		{
			PRINT_WIN32_ERROR(CryptCATAdminCalcHashFromFileHandle2);
			assert(FALSE);
			VECTOR_DESTROY(hash);
		}
	}

	return (Vector) { 0 };
}
static BOOL has_signature_in_catalog(ProcedureList* pKernel32, ProcedureList* pWintrust, LPCWSTR filepath)
{
	BOOL success = FALSE;

	PFN_CreateFileW create_file_w = *VECTOR_AT(pKernel32->procedures, PFN_CreateFileW, CREATE_FILE_W);
	HANDLE hFile = (HANDLE) create_file_w(
		filepath,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		PRINT_WIN32_ERROR(CreateFileW);
		assert(FALSE);
		return FALSE;
	}
	else
	{
		HCATADMIN hCatAdmin = NULL;
		GUID subSystem = DRIVER_ACTION_VERIFY;	// THis should maybe me NULL? Also - hash algorithm is default now so may produce false positives
		PFN_CryptCATAdminAcquireContext2 crypt_cat_admin_acquire_context_2 = *VECTOR_AT(pWintrust->procedures, 
																						PFN_CryptCATAdminAcquireContext2, 
																						CRYPT_CAT_ADMIN_ACQUIRE_CONTEXT_2);
		if (!crypt_cat_admin_acquire_context_2(&hCatAdmin, &subSystem, NULL, NULL, 0))
		{ 
			PRINT_WIN32_ERROR(CryptCATAdminAcquireContext2);
			assert(FALSE);
		}
		else
		{
			Vector hash = get_hash(pWintrust, hCatAdmin, hFile);
			if (hash.pData != NULL)
			{
				PFN_CryptCATAdminEnumCatalogFromHash 
					crypt_cat_admin_enum_catalog_from_hash = *VECTOR_AT(pWintrust->procedures,
																		PFN_CryptCATAdminEnumCatalogFromHash, 
																		CRYPT_CAT_ADMIN_ENUM_CATALOG_FROM_HASH);
				HCATINFO hCatInfo = (HCATINFO) crypt_cat_admin_enum_catalog_from_hash(hCatAdmin, 
																					  hash.pData, 
																					  (DWORD) VECTOR_SIZE(hash), 
																					  0, 
																					  NULL);
				success = hCatInfo != NULL;
				if (success)
				{
					PFN_CryptCATAdminReleaseCatalogContext 
						crypt_cat_admin_release_catalog_context = *VECTOR_AT(pWintrust->procedures, 
																			 PFN_CryptCATAdminReleaseCatalogContext, 
																			 CRYPT_CAT_ADMIN_RELEASE_CATALOG_CONTEXT);
					if (!crypt_cat_admin_release_catalog_context(hCatAdmin, hCatInfo, 0))
					{
						PRINT_WIN32_ERROR(CryptCATAdminReleaseCatalogContext);
						assert(FALSE);
					}
				}

				VECTOR_DESTROY(hash);
			}

			PFN_CryptCATAdminReleaseContext crypt_cat_admin_release_context = *VECTOR_AT(pWintrust->procedures, 
																					     PFN_CryptCATAdminReleaseContext, 
																					     CRYPT_CAT_ADMIN_RELEASE_CONTEXT);
			if (!crypt_cat_admin_release_context(hCatAdmin, 0))
			{
				PRINT_WIN32_ERROR(CryptCATAdminReleaseContext);
				assert(FALSE);
			}
		}
		
		PFN_CloseHandle close_handle = *VECTOR_AT(pKernel32->procedures, PFN_CloseHandle, CLOSE_HANDLE);
		close_handle(hFile);
	}

	return success;
}
static BOOL has_embedded_signature(ProcedureList* pKernel32, ProcedureList* pImagehlp, LPCWSTR filepath)
{
	PFN_CreateFileW create_file_w = *VECTOR_AT(pKernel32->procedures, PFN_CreateFileW, CREATE_FILE_W);
	HANDLE hFile = (HANDLE) create_file_w(
		filepath,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD count = 0;
		PFN_ImageEnumerateCertificates image_enumerate_certificates = *VECTOR_AT(pImagehlp->procedures,
																				 PFN_ImageEnumerateCertificates, 
																				 IMAGE_ENUMERATE_CERTIFICATES);
		if (!image_enumerate_certificates(hFile, CERT_SECTION_TYPE_ANY, &count, NULL, 0))
		{
			PRINT_WIN32_ERROR(ImageEnumerateCertificates);
			assert(FALSE);
		}
		
		PFN_CloseHandle close_handle = *VECTOR_AT(pKernel32->procedures, PFN_CloseHandle, CLOSE_HANDLE);
		close_handle(hFile);
		return count > 0;
	}
	else
	{
		PRINT_WIN32_ERROR(CreateFileW);
		assert(FALSE);
	}
	
	return FALSE;
}
static void append_backslash(WideString* pStr)
{
	wchar_t lastChar = WSTRING_BACK(*pStr);
	if (lastChar != L'\\')
	{
		WSTRING_PUSH_BACK(*pStr, L'\\');
	}
}
static Vector create_directories_stack(LPCWSTR mountPoint)
{
	Vector directories = VECTOR_CREATE(WideString, 0);

	WideString root = WSTRING_CREATE_FROM_LPCWSTR(mountPoint);
	append_backslash(&root);

	VECTOR_PUSH_BACK(directories, WideString, root);

	return directories;
}
static BOOL is_directory(const WIN32_FIND_DATAW* pData)
{
	return pData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
}
static BOOL is_sym_link(const WIN32_FIND_DATAW* pData)
{
	return pData->dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT;
}
static BOOL is_dots(const wchar_t* name)
{
	return (name[0] == L'.' && (name[1] == L'\0' || (name[1] == L'.' && name[2] == L'\0')));
}
static BOOL has_matching_extension(const WIN32_FIND_DATAW* pData, LPCWSTR extension)
{
	const wchar_t* ext = wcsrchr(pData->cFileName, L'.');
	if (ext != NULL)
	{
		return _wcsicmp(ext, extension) == 0;
	}

	return FALSE;
}
static BOOL find_next_file(ProcedureList* pKernel32, HANDLE hFind, WIN32_FIND_DATAW* pData)
{
	PFN_FindNextFileW find_next_file_w = *VECTOR_AT(pKernel32->procedures, PFN_FindNextFileW, FIND_NEXT_FILE_W);
	if (!find_next_file_w(hFind, pData))
	{
		PFN_GetLastError get_last_error = *VECTOR_AT(pKernel32->procedures, PFN_GetLastError, GET_LAST_ERROR);
		DWORD err = get_last_error();
		if (err != ERROR_NO_MORE_FILES)
		{
			printf("FindNextFileW failed with error: 0x%X", err);
			assert(FALSE);
		}
		
		return FALSE;
	}

	return TRUE;
}
static void do_search(ProcedureList* pKernel32, 
					  ProcedureList* pImagehlp, 
					  ProcedureList* pWintrust,
					  HANDLE hLog,
					  Vector* pDirectories, 
					  WideString* pCurrentDirectory, 
					  LPCWSTR extension, 
					  HANDLE hFind, 
					  WIN32_FIND_DATAW* pData)
{
	do
	{
		if (is_dots(pData->cFileName) || is_sym_link(pData))
		{
			continue;
		}

		if (is_directory(pData))
		{
			WideString subDirectory = WSTRING_CONCAT(*pCurrentDirectory, pData->cFileName);
			append_backslash(&subDirectory);
			VECTOR_PUSH_BACK(*pDirectories, WideString, subDirectory);
		}
		else
		{
			if (has_matching_extension(pData, extension))
			{
				WCHAR filepath[2048] = { 0 };
				swprintf(filepath, ARRAYSIZE(filepath), L"%ls%ls", WSTRING_C_STR(*pCurrentDirectory), pData->cFileName);
				if (!has_embedded_signature(pKernel32, pImagehlp, filepath) && 
					!has_signature_in_catalog(pKernel32, pWintrust, filepath))
				{
					WCHAR line[2560] = { 0 };
					swprintf(line, ARRAYSIZE(line), L"Found unsigned DLL at location: %ls%ls\n", 
							 WSTRING_C_STR(*pCurrentDirectory), 
							 pData->cFileName);
					write_to_file(hLog, line);
				}
			}
		}

	} while (find_next_file(pKernel32, hFind, pData));
}
static HANDLE start_search(ProcedureList* pKernel32, WideString* pDirectory, WIN32_FIND_DATAW* pOutData)
{
	WideString pattern = WSTRING_CONCAT(*pDirectory, L"*");

	PFN_FindFirstFileW find_first_file_w = *VECTOR_AT(pKernel32->procedures, PFN_FindFirstFileW, FINE_FIRST_FILE_W);
	HANDLE hFind = find_first_file_w(WSTRING_C_STR(pattern), pOutData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		PFN_GetLastError get_last_error = *VECTOR_AT(pKernel32->procedures, PFN_GetLastError, GET_LAST_ERROR);
		DWORD err = get_last_error();
		if (err == ERROR_ACCESS_DENIED)
		{
			printf("Access denied for directory: %ls. Skipping..", WSTRING_C_STR(*pDirectory));
		}
		else
		{
			printf("FindFirstFileW failed with error: 0x%X", err);
			assert(FALSE);
		}
	}
	WSTRING_DESTROY(pattern);

	return hFind;
}
static void find_file_with_ext(ProcedureList* pKernel32, 
							   ProcedureList* pImagehlp, 
							   ProcedureList* pWintrust, 
							   HANDLE hLog,
							   LPCWSTR mountPoint, 
							   LPCWSTR extension)
{
	Vector directories = create_directories_stack(mountPoint);

	while (!VECTOR_EMPTY(directories))
	{
		WideString directory = *VECTOR_BACK(directories, WideString);
		VECTOR_POP_BACK(directories);

		WIN32_FIND_DATAW data = { 0 };
		HANDLE hFind = start_search(pKernel32, &directory, &data);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			do_search(pKernel32, pImagehlp, pWintrust, hLog, &directories, &directory, extension, hFind, &data);
			FARPROC PFN_FindClose = *VECTOR_AT(pKernel32->procedures, FARPROC, FIND_CLOSE);
			PFN_FindClose(hFind);
		}
		
		WSTRING_DESTROY(directory);
	}

	VECTOR_DESTROY(directories);
}
static HANDLE get_first_hard_drive_volume(ProcedureList* pKernel32, LPWSTR outVolume, size_t outSize)
{
	assert(outSize == MAX_PATH);

	PFN_FindFirstVolumeW find_first_volume_w = *VECTOR_AT(pKernel32->procedures, PFN_FindFirstVolumeW, FIND_FIRST_VOLUME_W);
	HANDLE hVolume = find_first_volume_w(outVolume, outSize);
	if (hVolume == INVALID_HANDLE_VALUE)
	{
		PRINT_WIN32_ERROR(FindFirstVolumeW);
		assert(FALSE);
		return INVALID_HANDLE_VALUE;
	}
	
	return hVolume;
}
static DWORD get_required_volume_size(ProcedureList* pKernel32, LPCWSTR volume)
{
	DWORD size = 0;
	PFN_GetVolumePathNamesForVolumeNameW 
		get_volume_path_names_for_volume_name_w = *VECTOR_AT(pKernel32->procedures, 
															 PFN_GetVolumePathNamesForVolumeNameW, 
															 GET_VOLUME_PATH_NAMES_FOR_VOLUME_NAME_W);
	BOOL success = get_volume_path_names_for_volume_name_w(volume, NULL, 0, &size);
	if (!success)
	{
		DWORD err = GetLastError();
		if (err != ERROR_MORE_DATA)
		{
			printf("GetVolumePathNamesForVolumeNameW failed with: 0x%X", err);
			assert(FALSE);
			return 0;
		}
	}

	return size;
}
static Vector create_volume_names(ProcedureList* pKernel32, LPCWSTR volume)
{
	DWORD size = get_required_volume_size(pKernel32, volume);
	if (size == 0)
	{
		return (Vector) { 0 };
	}

	Vector names = VECTOR_CREATE(WCHAR, size);
	assert(VECTOR_SIZE(names) == size);

	PFN_GetVolumePathNamesForVolumeNameW 
		get_volume_path_names_for_volume_name_w = *VECTOR_AT(pKernel32->procedures, 
															 PFN_GetVolumePathNamesForVolumeNameW, 
															 GET_VOLUME_PATH_NAMES_FOR_VOLUME_NAME_W);
	BOOL success = get_volume_path_names_for_volume_name_w(volume, names.pData, VECTOR_SIZE(names), &size);
	if (!success)
	{
		PRINT_WIN32_ERROR(GetVolumePathNamesForVolumeNameW);
		assert(FALSE);
		VECTOR_DESTROY(names);
	}

	return names;
}
static BOOL find_next_hard_drive_volume(ProcedureList* pKernel32, HANDLE hVolume, WCHAR* pVolume, DWORD volumeLength)
{
	PFN_FindNextVolumeW find_next_volume_w = *VECTOR_AT(pKernel32->procedures, PFN_FindNextVolumeW, FIND_NEXT_VOLUME_W);
	if (!find_next_volume_w(hVolume, pVolume, volumeLength))
	{
		PFN_GetLastError get_last_error = *VECTOR_AT(pKernel32->procedures, PFN_GetLastError, FIND_NEXT_VOLUME_W);
		DWORD err = get_last_error();
		if (err != ERROR_NO_MORE_FILES)
		{
			printf("FindNextVolumeW failed with: 0x%X", err);
			assert(FALSE);
		}

		return FALSE;
	}

	return TRUE;
}
static void get_mount_points(ProcedureList* pKernel32, LPWSTR pOutMountPoints, size_t outSize)
{
	assert(pOutMountPoints != NULL);
	assert(outSize != 0);

	memset(pOutMountPoints, 0, outSize * sizeof(WCHAR));

	WCHAR volume[MAX_PATH] = { 0 };
	HANDLE hVolume = get_first_hard_drive_volume(pKernel32, volume, ARRAYSIZE(volume));

	size_t offset = 0;
	while (TRUE)
	{
		Vector names = create_volume_names(pKernel32, volume);
		if (names.pData != NULL)
		{
			const WCHAR* pName = names.pData;
			while (*pName != L'\0')
			{
				size_t charsToCopy = wcslen(pName) + 1;
				memcpy(pOutMountPoints + offset, pName, charsToCopy * sizeof(WCHAR));

				offset += charsToCopy;
				if (offset + 1 >= outSize)	// Multi_Sz check
				{
					assert(FALSE);
					break;
				}
			
				pName += wcslen(pName) + 1;
			}

			VECTOR_DESTROY(names);
		}


		if (!find_next_hard_drive_volume(pKernel32, hVolume, volume, ARRAYSIZE(volume)))
		{
			break;
		}
	}

	PFN_FindVolumeClose find_volume_close = *VECTOR_AT(pKernel32->procedures, PFN_FindVolumeClose, FIND_VOLUME_CLOSE);
	if (!find_volume_close(hVolume))
	{
		PRINT_WIN32_ERROR(FindVolumeClose);
		assert(FALSE);
	}
}
//
//
void execute_t1083()
{
	// Create Procedure List
	ProcedureList kernel32 = procedure_list_create("kernel32.dll", kernel32Procedures, ARRAYSIZE(kernel32Procedures));
	ProcedureList imagehlp = procedure_list_create("imagehlp.dll", imagehlpProcedures, ARRAYSIZE(imagehlpProcedures));
	ProcedureList wintrust = procedure_list_create("Wintrust.dll", wintrustProcedures, ARRAYSIZE(wintrustProcedures));

	
	WCHAR mountPoints[1024] = { 0 };
	get_mount_points(&kernel32, mountPoints, ARRAYSIZE(mountPoints));

	// Open log
	HANDLE hLog = open_log_file(L"LOG_T1083_");
	write_to_file(hLog, L"\n\nUnsigned DLLs:\n");

	LPCWSTR mountPoint = mountPoints;
	while (mountPoint[0] != L'\0')
	{
		find_file_with_ext(&kernel32, &imagehlp, &wintrust, hLog, mountPoint, L".dll");
		mountPoint += wcslen(mountPoint) + 1;
	}

	// Close Log
	PFN_CloseHandle close_handle = *VECTOR_AT(kernel32.procedures, PFN_CloseHandle, CLOSE_HANDLE);
	close_handle(hLog);
	// Destroy Procedure List
	procedure_list_destroy(&wintrust);
	procedure_list_destroy(&imagehlp);
	procedure_list_destroy(&kernel32);
}