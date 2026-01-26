#include "T1082.h"

#include "../misc/common.h"
#include "../misc/vector.h"

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
static void collect_hostname_account_info(HANDLE hLog)
{
	assert(hLog != INVALID_HANDLE_VALUE);

	WCHAR hostname[256] = { 0 };
	size_t hostnameLen = get_hostname(hostname, sizeof(hostname));
	
	WCHAR line[512] = { 0 };
	swprintf(line,
			 ARRAYSIZE(line),
			 L"\n\nFully Qualified DNS: %ls\n",
			 hostname);
	write_to_file(hLog, line);

	log_local_accounts(hLog);
}
static void log_cpu_and_memory(HANDLE hLog)
{
	SYSTEM_INFO info = { 0 };
	GetNativeSystemInfo(&info);

	if (info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
	{
		write_to_file(hLog, L"Architecture: x86-64\n");
	}
	else if (info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
	{
		write_to_file(hLog, L"Architecture: x86\n");
	}
	
	WCHAR processor[128] = { 0 };
	swprintf(processor, ARRAYSIZE(processor), L"Logical Processors: %lu\n", info.dwNumberOfProcessors);

	MEMORYSTATUSEX memory = { 0 };
	memory.dwLength = sizeof(MEMORYSTATUSEX);
	BOOL success = GlobalMemoryStatusEx(&memory);
	if (!success)
	{
		PRINT_WIN32_ERROR(GlobalMemoryStatusEx);
		assert(FALSE);
	}
	else
	{
		WCHAR memoryLine[512] = { 0 };
		swprintf(memoryLine,
				ARRAYSIZE(memoryLine),
				L"Total RAM (bytes): 0x%llX\n"
				L"Available RAM(bytes): 0x%llX\n"
				L"In Use: %lu%%\n",
				memory.ullTotalPhys,
				memory.ullAvailPhys,
				memory.dwMemoryLoad);
		write_to_file(hLog, memoryLine);
	}
}
static void log_hard_drives(HANDLE hLog)
{
	WCHAR volume[MAX_PATH] = { 0 };
	HANDLE hVolume = FindFirstVolumeW(volume, ARRAYSIZE(volume));
	if (hVolume == INVALID_HANDLE_VALUE)
	{
		PRINT_WIN32_ERROR(FindFirstVolumeW);
		assert(FALSE);
		return;
	}


	while (TRUE)
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
				break;
			}
		}

		Vector names = VECTOR_CREATE(WCHAR, size);
		assert(VECTOR_SIZE(names) == size);

		success = GetVolumePathNamesForVolumeNameW(volume, names.pData, VECTOR_SIZE(names), &size);
		if (!success)
		{
			PRINT_WIN32_ERROR(GetVolumePathNamesForVolumeNameW);
			assert(FALSE);
			break;
		}

		if (names.pData != L'\0')
		{
			WCHAR line[512] = { 0 };
			swprintf(line, ARRAYSIZE(line), L"Volume: %ls\n", volume);
			write_to_file(hLog, line);

			const WCHAR* pName = names.pData;
			while (*pName != L'\0')
			{
				WCHAR mountPoint[512] = { 0 };
				swprintf(mountPoint, ARRAYSIZE(mountPoint), L"MountPoint: %ls\n", pName);
				write_to_file(hLog, mountPoint);

				pName += wcslen(pName) + 1;
			}
		}

		VECTOR_DESTROY(names);

		if (!FindNextVolumeW(hVolume, volume, ARRAYSIZE(volume)))
		{
			DWORD err = GetLastError();
			if (err != ERROR_NO_MORE_FILES)
			{
				printf("FindNextVolumeW failed with: 0x%X", err);
				assert(FALSE);
			}

			break;
		}
	}
	
	if (!FindVolumeClose(hVolume))
	{
		PRINT_WIN32_ERROR(FindVolumeClose);
		assert(FALSE);
	}
}
static void collect_hardware_info(HANDLE hLog)
{
	write_to_file(hLog, L"\n\nHardware Info\n");

	log_cpu_and_memory(hLog);

	log_hard_drives(hLog);
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
	collect_hostname_account_info(hLog);

	// CPU, Ram size, Connected DISKS
	collect_hardware_info(hLog);

	// Language, Locale

	// Log details

	// Destroy ProcedueList

	CloseHandle(hLog);
}