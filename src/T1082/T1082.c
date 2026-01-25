#include "T1082.h"

#include "../misc/common.h"

#include "assert.h"
#include "stdio.h"

#define WIN_LEAN_AND_MEAN
#include "Windows.h"


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
			 L"Product: %ls\nVersion: %ls\nBuild: %ls\nUBR: %i", 
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
//
//
void execute_t1082()
{
	HANDLE hLog = open_log_file(L"LOG_T1082_");

	// Create ProcedueList

	// OS and build
	collect_os_info(hLog);

	// Hostname, domain membership

	// CPU, Ram size, Connected DISKS

	// Language, Locale

	// Log details

	// Destroy ProcedueList

	CloseHandle(hLog);
}