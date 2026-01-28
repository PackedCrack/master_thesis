#include "T1083.h"

#include "../misc/common.h"
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


// TODO: REMOVE ME
#pragma comment(lib, "imagehlp.lib")
#pragma comment(lib, "wintrust.lib")
#pragma comment(lib, "crypt32.lib")


// Kernel32.dll
#define CLOSE_HANDLE 0
static const char* kernel32Procedures[1] = { "CloseHandle" };


static DWORD get_hash_size(HCATADMIN hCatAdmin, HANDLE hFile)
{
	DWORD cbHash = 0;
	if (!CryptCATAdminCalcHashFromFileHandle2(hCatAdmin, hFile, &cbHash, NULL, 0))
	{
		return 0;
	}

	return cbHash;
}
static Vector get_hash(HCATADMIN hCatAdmin, HANDLE hFile)
{
	// 3) Compute hash
	DWORD size = get_hash_size(hCatAdmin, hFile);
	if (size > 0)
	{
		Vector hash = VECTOR_CREATE(BYTE, size);
		if (CryptCATAdminCalcHashFromFileHandle2(hCatAdmin, hFile, &size, hash.pData, 0))
		{
			return hash;
		}
		else
		{
			PRINT_WIN32_ERROR(CryptCATAdminReleaseContext);
			assert(FALSE);
			VECTOR_DESTROY(hash);
		}
	}
	else
	{
		PRINT_WIN32_ERROR(CryptCATAdminReleaseContext);
		assert(FALSE);
	}

	return (Vector) { 0 };
}
static BOOL has_signature_in_catalog(LPCWSTR filepath)
{
	BOOL success = FALSE;

	HANDLE hFile = CreateFileW(
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
		GUID subSystem = DRIVER_ACTION_VERIFY;
		if (!CryptCATAdminAcquireContext2(&hCatAdmin, &subSystem, NULL, NULL, 0))
		{ 
			PRINT_WIN32_ERROR(CryptCATAdminAcquireContext2);
			assert(FALSE);
		}
		else
		{

			Vector hash = get_hash(hCatAdmin, hFile);
			if (hash.pData != NULL)
			{
				HCATINFO hCatInfo = CryptCATAdminEnumCatalogFromHash(hCatAdmin, hash.pData, (DWORD) VECTOR_SIZE(hash), 0, NULL);
				success = hCatInfo != NULL;
				if (success)
				{
					if (!CryptCATAdminReleaseCatalogContext(hCatAdmin, hCatInfo, 0))
					{
						PRINT_WIN32_ERROR(CryptCATAdminReleaseCatalogContext);
						assert(FALSE);
					}
				}

				VECTOR_DESTROY(hash);
			}

			if (!CryptCATAdminReleaseContext(hCatAdmin, 0))
			{
				PRINT_WIN32_ERROR(CryptCATAdminReleaseContext);
				assert(FALSE);
			}
		}
		
		CloseHandle(hFile);
	}

	return success;
}
static BOOL has_embedded_signature(LPCWSTR filepath)
{
	HANDLE hFile = CreateFileW(
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
		if (!ImageEnumerateCertificates(hFile, CERT_SECTION_TYPE_ANY, &count, NULL, 0))
		{
			PRINT_WIN32_ERROR(ImageEnumerateCertificates);
			assert(FALSE);
		}
		
		CloseHandle(hFile);
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
static BOOL find_next_file(HANDLE hFind, WIN32_FIND_DATAW* pData)
{
	if (!FindNextFileW(hFind, pData))
	{
		DWORD err = GetLastError();
		if (err != ERROR_NO_MORE_FILES)
		{
			printf("FindNextFileW failed with error: 0x%X", err);
			assert(FALSE);
		}
		
		return FALSE;
	}

	return TRUE;
}
static void do_search(Vector* pDirectories, WideString* pCurrentDirectory, LPCWSTR extension, HANDLE hFind, WIN32_FIND_DATAW* pData)
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
				WCHAR filepath[512] = { 0 };
				swprintf(filepath, ARRAYSIZE(filepath), L"%ls%ls", WSTRING_C_STR(*pCurrentDirectory), pData->cFileName);
				if (has_embedded_signature(filepath) || has_signature_in_catalog(filepath))
				{
					printf("Found file: %ls. In directory: %ls\n", pData->cFileName, WSTRING_C_STR(*pCurrentDirectory));
					int a = 10;
				}
			}
		}

	} while (find_next_file(hFind, pData));
}
static HANDLE start_search(WideString* pDirectory, WIN32_FIND_DATAW* pOutData)
{
	WideString pattern = WSTRING_CONCAT(*pDirectory, L"*");
	HANDLE hFind = FindFirstFileW(WSTRING_C_STR(pattern), pOutData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		DWORD err = GetLastError();
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
static void find_file_with_ext(LPCWSTR mountPoint, LPCWSTR extension)
{
	Vector directories = create_directories_stack(mountPoint);

	while (!VECTOR_EMPTY(directories))
	{
		WideString directory = *VECTOR_BACK(directories, WideString);
		VECTOR_POP_BACK(directories);

		WIN32_FIND_DATAW data = { 0 };
		HANDLE hFind = start_search(&directory, &data);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			do_search(&directories, &directory, extension, hFind, &data);
			FindClose(hFind);
		}
		
		WSTRING_DESTROY(directory);
	}

	VECTOR_DESTROY(directories);
}
static HANDLE get_first_hard_drive_volume(LPWSTR outVolume, size_t outSize)
{
	assert(outSize == MAX_PATH);
	HANDLE hVolume = FindFirstVolumeW(outVolume, outSize);
	if (hVolume == INVALID_HANDLE_VALUE)
	{
		PRINT_WIN32_ERROR(FindFirstVolumeW);
		assert(FALSE);
		return INVALID_HANDLE_VALUE;
	}
	
	return hVolume;
}
static DWORD get_required_volume_size(LPCWSTR volume)
{
	DWORD size = 0;
	BOOL success = GetVolumePathNamesForVolumeNameW(volume, NULL, 0, &size);
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
static Vector create_volume_names(LPCWSTR volume)
{
	DWORD size = get_required_volume_size(volume);
	if (size == 0)
	{
		return (Vector) { 0 };
	}

	Vector names = VECTOR_CREATE(WCHAR, size);
	assert(VECTOR_SIZE(names) == size);

	BOOL success = GetVolumePathNamesForVolumeNameW(volume, names.pData, VECTOR_SIZE(names), &size);
	if (!success)
	{
		PRINT_WIN32_ERROR(GetVolumePathNamesForVolumeNameW);
		assert(FALSE);
		VECTOR_DESTROY(names);
	}

	return names;
}
static BOOL find_next_hard_drive_volume(HANDLE hVolume, WCHAR* pVolume, DWORD volumeLength)
{
	if (!FindNextVolumeW(hVolume, pVolume, volumeLength))
	{
		DWORD err = GetLastError();
		if (err != ERROR_NO_MORE_FILES)
		{
			printf("FindNextVolumeW failed with: 0x%X", err);
			assert(FALSE);
		}

		return FALSE;
	}

	return TRUE;
}
static void get_mount_points(LPWSTR pOutMountPoints, size_t outSize)
{
	assert(pOutMountPoints != NULL);
	assert(outSize != 0);

	memset(pOutMountPoints, 0, outSize * sizeof(WCHAR));

	WCHAR volume[MAX_PATH] = { 0 };
	HANDLE hVolume = get_first_hard_drive_volume(volume, ARRAYSIZE(volume));

	size_t offset = 0;
	while (TRUE)
	{
		Vector names = create_volume_names(volume);
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


		if (!find_next_hard_drive_volume(hVolume, volume, ARRAYSIZE(volume)))
		{
			break;
		}
	}

	if (!FindVolumeClose(hVolume))
	{
		PRINT_WIN32_ERROR(FindVolumeClose);
		assert(FALSE);
	}
}
//
//
void execute_t1083()
{
	WCHAR mountPoints[1024] = { 0 };
	get_mount_points(mountPoints, ARRAYSIZE(mountPoints));

	LPCWSTR mountPoint = mountPoints;
	while (mountPoint[0] != L'\0')
	{
		if (lstrcmpW(mountPoint, L"E:\\") == 0)
		{
			find_file_with_ext(mountPoint, L".dll");
		}
		mountPoint += wcslen(mountPoint) + 1;
	}


	// Create Procedure List
	ProcedureList kernel32 = procedure_list_create("kernel32.dll", kernel32Procedures, ARRAYSIZE(kernel32Procedures));

	// Open log
	HANDLE hLog = open_log_file(L"LOG_T1083_");


	// Close Log
	FARPROC PFN_CloseHandle = *VECTOR_AT(kernel32.procedures, FARPROC, CLOSE_HANDLE);
	PFN_CloseHandle(hLog);
	// Destroy Procedure List
	procedure_list_destroy(&kernel32);
}