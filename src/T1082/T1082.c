#include "T1082.h"

#include "../misc/common.h"

#include "assert.h"
#include "stdio.h"

#define WIN_LEAN_AND_MEAN
#include <Windows.h>
#include <lm.h>

// TODO: REMOVE ME
#pragma comment(lib, "Netapi32.lib")


static DWORD get_registry_dword_value(LPWSTR pValue)
{
	DWORD data = 0;
	DWORD cbData = sizeof(DWORD);
	LSTATUS status = RegGetValueW(HKEY_LOCAL_MACHINE,
								  L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
								  pValue,
								  RRF_RT_REG_DWORD,
								  NULL,
								  &data,
								  &cbData);
	assert(status == ERROR_SUCCESS);

	return data;
}
static void get_registry_str_value(LPWSTR pValue, LPWSTR pOutData, DWORD outBufferSize)
{
	LSTATUS status = RegGetValueW(HKEY_LOCAL_MACHINE,
								  L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
								  pValue,
								  RRF_RT_REG_SZ,
								  NULL,
								  pOutData,
								  &outBufferSize);
	assert(status == ERROR_SUCCESS);
}
static void collect_os_info(HANDLE hLog)
{
	assert(hLog != INVALID_HANDLE_VALUE);


	WCHAR name[256] = { 0 };
	get_registry_str_value(L"ProductName", name, sizeof(name));
	WCHAR version[64] = { 0 };
	get_registry_str_value(L"DisplayVersion", version, sizeof(version));
	WCHAR build[64] = { 0 };
	get_registry_str_value(L"CurrentBuildNumber", build, sizeof(build));

	DWORD updateBuildRevision = get_registry_dword_value(L"UBR");

	WCHAR output[512] = { 0 };
	swprintf(output, 
			 ARRAYSIZE(output), 
			 L"Product: %ls\nVersion: %ls\nBuild: %ls\nUBR: %i\n", 
			 name,
			 version, 
			 build, 
			 updateBuildRevision);

	BOOL success = WriteFile(hLog, output, wcslen(output) * sizeof(WCHAR), NULL, NULL);
	if (!success)
	{
		PRINT_WIN32_ERROR(WriteFile);
		assert(FALSE);
	}
}
static size_t get_hostname(LPWSTR pOut, size_t outSize)
{
	assert(pOut != NULL);
	assert(outSize > 256);

	DWORD size = outSize / sizeof(WCHAR);
	BOOL success = GetComputerNameExW(ComputerNameDnsFullyQualified,
									  pOut,
									  &size);

	if (!success)
	{
		PRINT_WIN32_ERROR(GetComputerNameExW);
		assert(FALSE);
		return 0;
	}

	return wcslen(pOut);
}
static LPWSTR get_privilege(DWORD priv)
{
	if (priv == USER_PRIV_GUEST)
	{
		return L"Guest";
	}
	else if (priv == USER_PRIV_USER)
	{
		return L"User";
	}
	else if (priv == USER_PRIV_ADMIN)
	{
		return L"Administrator";
	}

	return L"Unknown Privilege";
}
static void log_local_accounts(HANDLE hLog)
{
	LPBYTE* pBuffer = NULL;
	DWORD entriesRead = 0;
	DWORD totalEntires = 0;
	DWORD resume = 0;

	while (TRUE)
	{
		DWORD result = NetUserEnum(NULL,
								   1,
								   FILTER_NORMAL_ACCOUNT,
								   &pBuffer,
								   MAX_PREFERRED_LENGTH,
								   &entriesRead,
								   &totalEntires,
								   &resume);

		if (result == ERROR_ACCESS_DENIED)
		{
			write_to_file(hLog, L"NetUserEnum failed with: ERROR_ACCESS_DENIED\n");
			break;
		}

		if (result == NERR_Success || result == ERROR_MORE_DATA)
		{
			USER_INFO_1* pInfo = (USER_INFO_1*) pBuffer;
			for (DWORD i = 0; i < entriesRead; ++i)
			{
				WCHAR line[2048] = { 0 };
				LPWSTR privilege = get_privilege(pInfo->usri1_priv);
				swprintf(line, 
						 ARRAYSIZE(line), 
						 L"\nAccount Name: %ls\nPrivilege: %ls\nComment: %ls\n",
						 pInfo->usri1_name,
						 privilege,
						 pInfo->usri1_comment);

				write_to_file(hLog, line);

				if (pInfo->usri1_password != NULL)
				{
					WCHAR password[128] = { 0 };
					swprintf(password,
							 ARRAYSIZE(password),
							 L"Password: %ls\nPassword Age: %d",
							 pInfo->usri1_password,
							 pInfo->usri1_password_age);
					write_to_file(hLog, password);
				}

				if (pInfo->usri1_home_dir != NULL)
				{
					if (wcslen(pInfo->usri1_home_dir) > 0)
					{
						WCHAR home[128] = { 0 };
						swprintf(home,
								 ARRAYSIZE(home),
								 L"Home Directory: %ls\n",
								 pInfo->usri1_home_dir);
						write_to_file(hLog, home);
					}
				}

				if (pInfo->usri1_script_path != NULL)
				{
					if (wcslen(pInfo->usri1_script_path) > 0)
					{
						WCHAR script[128] = { 0 };
						swprintf(script,
								 ARRAYSIZE(script),
								 L"Script Path: %ls\n",
								 pInfo->usri1_script_path);
						write_to_file(hLog, script);
					}
				}

				DWORD flags = pInfo->usri1_flags;
				if (flags & UF_ACCOUNTDISABLE)
				{
					write_to_file(hLog, L"Account Disabled\n");
				}
				if (flags & UF_PASSWD_NOTREQD)
				{
					write_to_file(hLog, L"No Password Required\n");
				}
				if (flags & UF_DONT_REQUIRE_PREAUTH)
				{
					write_to_file(hLog, L"Kerberos Not Required\n");
				}

				++pInfo;
			}


			if (pBuffer)
			{ 
				NetApiBufferFree(pBuffer);
				pInfo = NULL;
			}

			if (result == NERR_Success)
			{
				break;
			}
		}
	}
}
static void collect_account_and_networking(HANDLE hLog)
{
	assert(hLog != INVALID_HANDLE_VALUE);

	WCHAR hostname[256] = { 0 };
	size_t hostnameLen = get_hostname(hostname, sizeof(hostname));

	log_local_accounts(hLog);


	int a = 10;
}
//
//
void execute_t1082()
{
	HANDLE hLog = open_log_file(L"LOG_T1082_");

	// Create ProcedueList

	// OS and build
	collect_os_info(hLog);

	// Hostname, domain membership
	collect_account_and_networking(hLog);

	// CPU, Ram size, Connected DISKS

	// Language, Locale

	// Log details

	// Destroy ProcedueList

	CloseHandle(hLog);
}