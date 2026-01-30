#include "T1547.001.h"

#include "../misc/str.h"
#include "../runtime_linking.h"

#include <assert.h>
#include <stdio.h>

#define WIN_LEAN_AND_MEAN
#include <Windows.h>
#include <wincrypt.h>

// advapi32.dll
#define REG_CREATE_KEY_EX_W 0
#define REG_SET_VALUE_EX_A 1
#define REG_CLOSE_KEY 2
static const char* advapi32Procedures[3] = { "RegCreateKeyExW", "RegSetValueExA", "RegCloseKey" };
// Crypt32.dll
#define CRYPT_BINARY_TO_STRING_A 0
static const char* crypt32Procedures[1] = { "CryptBinaryToStringA" };

static HKEY create_key(ProcedureList* pAdvapi32, HKEY key, LPCWSTR subkey)
{
	HKEY hKey = NULL;
	DWORD disposition = 0;

	FARPROC PFN_RegCreateKeyExW = *VECTOR_AT(pAdvapi32->procedures, FARPROC, REG_CREATE_KEY_EX_W);
	LSTATUS status = PFN_RegCreateKeyExW(key,
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
	FARPROC PFN_CryptBinaryToStringA = *VECTOR_AT(pCrypt32->procedures, FARPROC, CRYPT_BINARY_TO_STRING_A);
	if (!PFN_CryptBinaryToStringA(string, (DWORD) len, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, &size))
	{
		return (String) { 0 };
	}

	Vector buffer = VECTOR_CREATE(char, size);
	BOOL b = PFN_CryptBinaryToStringA(string, (DWORD) len, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, buffer.pData, &size);
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


	FARPROC PFN_RegSetValueExA = *VECTOR_AT(pAdvapi32->procedures, FARPROC, REG_SET_VALUE_EX_A);
	if (PFN_RegSetValueExA(hKey, 
						   "__Pseudo_Malware", 
						   0, 
						   REG_SZ,
						   encodedFilepath.array.pData,
						   STRING_SIZE(encodedFilepath) + 1) != ERROR_SUCCESS)
	{
		assert(FALSE);
	}

	FARPROC PFN_RegCloseKey = *VECTOR_AT(pAdvapi32->procedures, FARPROC, REG_CLOSE_KEY);
	if (PFN_RegCloseKey(hKey) != ERROR_SUCCESS)
	{
		assert(FALSE);
	}
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
//
//
void execute_t1574_001(char** argv)
{
	ProcedureList advapi32 = procedure_list_create("advapi32.dll", advapi32Procedures, ARRAYSIZE(advapi32Procedures));
	ProcedureList crypt32 = procedure_list_create("Crypt32.dll", crypt32Procedures, ARRAYSIZE(crypt32Procedures));

	add_run_key(&advapi32, &crypt32, argv);

	procedure_list_destroy(&crypt32);
	procedure_list_destroy(&advapi32);
}