#include "T1057.h"

#include "../misc/common.h"
#include "../misc/function_pointers.h"
#include "../misc/vector.h"
#include "../misc/wstr.h"
#include "../runtime_linking.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define WIN_LEAN_AND_MEAN
#include <Windows.h>
#include <Psapi.h>


typedef DWORD PID;

// Kernel32.dll
#define CLOSE_HANDLE 0
#define K32_ENUM_PROCESSES 1
#define OPEN_PROCESS 2
#define K32_GET_MODULE_BASE_NAME_W 3
#define GET_LAST_ERROR 4
static const char* kernel32Procedures[5] = { "CloseHandle", "K32EnumProcesses", "OpenProcess", "K32GetModuleBaseNameW",
												"GetLastError" };

// advapi32.dll
#define OPEN_PROCESS_TOKEN 0
#define GET_TOKEN_INFORMATION 1
#define LOOKUP_PRIVILEGE_NAME_W 2
static const char* advapi32Procedures[3] = { "OpenProcessToken", "GetTokenInformation", "LookupPrivilegeNameW" };


// https://github.com/nand0san/av_detect/blob/main/processes.csv
#define NUM_AV_PROCESSES 44
static const wchar_t* avProcesses[NUM_AV_PROCESSES] = {
	L"aswidsagent.exe",
	L"avastsvc.exe",
	L"avastui.exe",
	L"avgnt.exe",
	L"avguard.exe",
	L"avp.exe",
	L"avpui.exe",
	L"bdagent.exe",
	L"bdntwrk.exe",
	L"ccsvchst.exe",
	L"clientcommunicationservice.exe",
	L"clientlogservice.exe",
	L"clientsolutionframework.exe",
	L"coreserviceshell.exe",
	L"egui.exe",
	L"klwtblfs.exe",
	L"macmnsvc.exe",
	L"masvc.exe",
	L"mcshield.exe",
	L"mfemms.exe",
	L"mpdefendercoreservice.exe",
	L"msascuil.exe",
	L"msmpeng.exe",
	L"nortonsecurity.exe",
	L"ns.exe",
	L"nsservice.exe",
	L"ntrtscan.exe",
	L"pavfnsvr.exe",
	L"pavsrv.exe",
	L"realtimescanservice.exe",
	L"rtvscan.exe",
	L"samplingservice.exe",
	L"savservice.exe",
	L"shstat.exe",
	L"sophosav.exe",
	L"sophosclean.exe",
	L"sophosui.exe",
	L"tmlisten.exe",
	L"tmntsrv.exe",
	L"tmproxy.exe",
	L"updatesrv.exe",
	L"vsserv.exe",
	L"windefend.exe",
	L"wscservice.exe",
};
#define NUM_EDR_PROCESSES 32
static const wchar_t* edrProcesses[NUM_EDR_PROCESSES] = {
	L"csfalconcontainer.exe",
	L"csfalcondaterepair.exe",
	L"csfalconservice.exe",
	L"cyserver.exe",
	L"cyveraconsole.exe",
	L"cyveraservice.exe",
	L"cyvragentsvc.exe",
	L"cyvrfsflt.exe",
	L"ekrn.exe",
	L"elastic-agent.exe",
	L"elastic-endpoint.exe",
	L"endpoint-security.exe",
	L"endpointbasecamp.exe",
	L"firesvc.exe",
	L"firetray.exe",
	L"ir_agent.exe",
	L"mssense.exe",
	L"psanhost.exe",
	L"sentinelagent.exe",
	L"sentinelctl.exe",
	L"sentinelmemoryscanner.exe",
	L"sentinelservicehost.exe",
	L"sentinelstaticengine.exe",
	L"sentinelstaticenginescanner.exe",
	L"sophossps.exe",
	L"tanclient.exe",
	L"taniumclient.exe",
	L"traps.exe",
	L"trapsagent.exe",
	L"trapsd.exe",
	L"wrsa.exe",
	L"xagt.exe",
};
#define NUM_FIREWALL_PROCESSES 3
static const wchar_t* firewallProcesses[NUM_FIREWALL_PROCESSES] = {
	L"fw.exe",
	L"mfemactl.exe",
	L"personalfirewallservice.exe",
};
#define NUM_VIRT_PROCESSES 7
static const wchar_t* virtProcesses[NUM_VIRT_PROCESSES] = {
	L"vgauthservice.exe",
	L"vm3dservice.exe",
	//L"vmnat.exe",				// HOST SIDE
	// L"vmnetdhcp.exe",		// HOST SIDE
	L"vmtoolsd.exe", 
	// L"vmware-authd.exe",		// HOST SIDE
	L"vmware-tray.exe",
	L"vmware-usbarbitrator64.exe",
	L"vboxservice.exe",			// https://github.com/ayoubfaouzi/al-khaser/blob/master/al-khaser/AntiVM/VirtualBox.cpp
	L"vboxtray.exe",			// https://github.com/ayoubfaouzi/al-khaser/blob/master/al-khaser/AntiVM/VirtualBox.cpp
	// L"wsl.exe",				// HOST SIDE
	// L"wslhost.exe",			// HOST SIDE
	// L"wslservice.exe",		// HOST SIDE
};
#define NUM_CREDENTIAL_PROCESSES 4
static const wchar_t* credentialProcesses[NUM_CREDENTIAL_PROCESSES] = {
	L"1password.exe",
	L"bitwarden.exe",
	L"keepass.exe",
	L"keepassxc.exe",
};

static Vector create_pids(ProcedureList* pKernel32, DWORD count)
{
	Vector tmp = VECTOR_CREATE(PID, count);

	DWORD byteSize = VECTOR_SIZE(tmp) * sizeof(PID);
	DWORD neededByteSize = 0;
	PFN_K32EnumProcesses k32_enum_processes = *VECTOR_AT(pKernel32->procedures, PFN_K32EnumProcesses, K32_ENUM_PROCESSES);
	if (!k32_enum_processes(tmp.pData, byteSize, &neededByteSize))
	{
		PRINT_WIN32_ERROR(K32EnumProcesses);
		assert(FALSE);
		VECTOR_DESTROY(tmp);
		return (Vector) { 0 };
	}

	DWORD neededCount = neededByteSize / sizeof(PID);
	if (neededCount >= count)
	{
		VECTOR_DESTROY(tmp);
		return create_pids(pKernel32, count * 2);
	}
	else
	{
		Vector pids = VECTOR_CREATE(PID, neededCount);
		memcpy(pids.pData, tmp.pData, neededByteSize);
		VECTOR_DESTROY(tmp);
		return pids;
	}
}
static HANDLE open_process(ProcedureList* pKernel32, PID pid)
{
	if (pid == 0)	// hack
	{
		return NULL;
	}

	PFN_OpenProcess open_process = *VECTOR_AT(pKernel32->procedures, PFN_OpenProcess, OPEN_PROCESS);
	HANDLE hProcess = open_process(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
	if (hProcess == NULL)
	{
		PFN_GetLastError get_last_error = *VECTOR_AT(pKernel32->procedures, PFN_GetLastError, GET_LAST_ERROR);
		DWORD err = get_last_error();
		if (err != ERROR_ACCESS_DENIED)
		{
			printf("OpenProcess failed with error: 0x%lX", err);
			assert(FALSE);
		}
	}

	return hProcess;
}
static BOOL is_one_of(LPCWSTR name, const wchar_t* processes[], size_t numProcesses)
{
	for (size_t i = 0; i < numProcesses; ++i)
	{
		if (_wcsicmp(processes[i], name) == 0)
		{
			return TRUE;
		}
	}

	return FALSE;
}
static BOOL is_av_process(LPCWSTR name)
{
	return is_one_of(name, avProcesses, NUM_AV_PROCESSES);
}
static BOOL is_edr_process(LPCWSTR name)
{
	return is_one_of(name, edrProcesses, NUM_EDR_PROCESSES);
}
static BOOL is_firewall_process(LPCWSTR name)
{
	return is_one_of(name, firewallProcesses, NUM_FIREWALL_PROCESSES);
}
static BOOL is_virt_process(LPCWSTR name)
{
	return is_one_of(name, virtProcesses, NUM_VIRT_PROCESSES);
}
static BOOL is_credential_process(LPCWSTR name)
{
	return is_one_of(name, credentialProcesses, NUM_CREDENTIAL_PROCESSES);
}
static WideString create_process_name(ProcedureList* pKernel32, HANDLE hProcess)
{
	PFN_K32GetModuleBaseNameW k32_get_module_base_name_w = *VECTOR_AT(pKernel32->procedures, 
																	  PFN_K32GetModuleBaseNameW, 
																	  K32_GET_MODULE_BASE_NAME_W);
	
	WCHAR tmpName[1024] = { 0 };
	DWORD len = k32_get_module_base_name_w(hProcess, NULL, tmpName, ARRAYSIZE(tmpName));
	if (len == 0)
	{
		PFN_GetLastError get_last_error = *VECTOR_AT(pKernel32->procedures, PFN_GetLastError, GET_LAST_ERROR);
		DWORD err = get_last_error();
		// https://devblogs.microsoft.com/oldnewthing/20150716-00/?p=45131
		if (err != ERROR_INVALID_HANDLE && err != ERROR_PARTIAL_COPY)	// partial copy can supposedly happen when 64bit reads 32bit or vice versa
		{
			PRINT_WIN32_ERROR(K32GetModuleBaseNameW);
			assert(FALSE);
		}
		return (WideString) { 0 };
	}

	return WSTRING_CREATE_FROM_LPCWSTR(tmpName);
}
static BOOL running_in_sandbox(HANDLE hLog, LPCWSTR name)
{
	if (is_virt_process(name))
	{
		WCHAR line[512] = { 0 };
		swprintf(line, ARRAYSIZE(line), L"Found virtualization process: %ls\n", name);
		write_to_file(hLog, line);

		return TRUE;
	}

	return FALSE;
}
static BOOL is_anti_malware(HANDLE hLog, LPCWSTR name)
{
	if (is_av_process(name))
	{
		WCHAR line[512] = { 0 };
		swprintf(line, ARRAYSIZE(line), L"Found anti-virus process: %ls\n", name);
		write_to_file(hLog, line);
		return TRUE;
	}
	else if (is_edr_process(name))
	{
		WCHAR line[512] = { 0 };
		swprintf(line, ARRAYSIZE(line), L"Found endpoint detection and response process: %ls\n", name);
		write_to_file(hLog, line);
		return TRUE;
	}
	
	return FALSE;
}
static HANDLE get_process_token(ProcedureList* pAdvApi32, HANDLE hProcess)
{
	HANDLE hToken = NULL;
	PFN_OpenProcessToken open_process_token = *VECTOR_AT(pAdvApi32->procedures, PFN_OpenProcessToken, OPEN_PROCESS_TOKEN);
	if (!open_process_token(hProcess, TOKEN_QUERY, &hToken))
	{
		PRINT_WIN32_ERROR(OpenProcessToken);
		assert(FALSE);
		hToken = NULL;
	}

	return hToken;
}
static DWORD get_token_info_length(ProcedureList* pKernel32, ProcedureList* pAdvApi32, HANDLE hToken)
{
	DWORD size = 0;
	PFN_GetTokenInformation get_token_information = *VECTOR_AT(pAdvApi32->procedures, PFN_GetTokenInformation, GET_TOKEN_INFORMATION);
	if (!get_token_information(hToken, TokenPrivileges, NULL, 0, &size))
	{
		PFN_GetLastError get_last_error = *VECTOR_AT(pKernel32->procedures, PFN_GetLastError, GET_LAST_ERROR);
		DWORD err = get_last_error();
		if (err != ERROR_INSUFFICIENT_BUFFER)
		{
			PRINT_WIN32_ERROR(GetTokenInformation);
			assert(FALSE);
		}
	}

	return size;
}
static Vector create_token_info(ProcedureList* pKernel32, ProcedureList* pAdvApi32, HANDLE hToken)
{
	DWORD size = get_token_info_length(pKernel32, pAdvApi32, hToken);
	Vector tokenInfo = VECTOR_CREATE(BYTE, size);

	PFN_GetTokenInformation get_token_information = *VECTOR_AT(pAdvApi32->procedures, PFN_GetTokenInformation, GET_TOKEN_INFORMATION);
	if (!get_token_information(hToken, TokenPrivileges, tokenInfo.pData, VECTOR_SIZE(tokenInfo), &size))
	{
		PRINT_WIN32_ERROR(GetTokenInformation);
		assert(FALSE);
		return (Vector) { 0 };
	}

	return tokenInfo;
}
static DWORD get_privilege_name_length(ProcedureList* pKernel32, ProcedureList* pAdvApi32, PLUID_AND_ATTRIBUTES pPrivilege)
{
	DWORD length = 0;
	PFN_LookupPrivilegeNameW lookup_privilege_name_w = *VECTOR_AT(pAdvApi32->procedures, 
																  PFN_LookupPrivilegeNameW, 
																  LOOKUP_PRIVILEGE_NAME_W);
	if (!lookup_privilege_name_w(NULL, &pPrivilege->Luid, NULL, &length))
	{
		PFN_GetLastError get_last_error = *VECTOR_AT(pKernel32->procedures, PFN_GetLastError, GET_LAST_ERROR);
		DWORD err = get_last_error();
		if (err != ERROR_INSUFFICIENT_BUFFER)
		{
			PRINT_WIN32_ERROR(LookupPrivilegeNameW);
			assert(FALSE);
		}
	}

	return length;
}
static WideString create_privilege_name(ProcedureList* pKernel32, ProcedureList* pAdvApi32, PLUID_AND_ATTRIBUTES pPrivilege)
{
	DWORD length = get_privilege_name_length(pKernel32, pAdvApi32, pPrivilege);
	if (length < 1)
	{
		return (WideString) { 0 };
	}

	Vector buffer = VECTOR_CREATE(WCHAR, length + 1);
	PFN_LookupPrivilegeNameW lookup_privilege_name_w = *VECTOR_AT(pAdvApi32->procedures, 
																  PFN_LookupPrivilegeNameW, 
																  LOOKUP_PRIVILEGE_NAME_W);
	DWORD size = VECTOR_SIZE(buffer);
	if (!lookup_privilege_name_w(NULL, &pPrivilege->Luid, buffer.pData, &size))
	{
		PRINT_WIN32_ERROR(LookupPrivilegeNameW);
		assert(FALSE);
		VECTOR_DESTROY(buffer);
		return (WideString) { 0 };
	}
	
	WideString name = WSTRING_CREATE_FROM_LPCWSTR(buffer.pData);
	VECTOR_DESTROY(buffer);
	return name;
}
static void log_process_rights(ProcedureList* pKernel32, ProcedureList* pAdvApi32, HANDLE hLog, HANDLE hProcess, LPCWSTR processName)
{
	HANDLE hToken = get_process_token(pAdvApi32, hProcess);
	if (hToken != NULL)
	{
		Vector tokenInfo = create_token_info(pKernel32, pAdvApi32, hToken);
		if (tokenInfo.pData != NULL)
		{
			PTOKEN_PRIVILEGES tp = (PTOKEN_PRIVILEGES) tokenInfo.pData;
			for (DWORD i = 0; i < tp->PrivilegeCount; ++i)
			{
				PLUID_AND_ATTRIBUTES pPrivilege = &tp->Privileges[i];
				WideString name = create_privilege_name(pKernel32, pAdvApi32, pPrivilege);

				BOOL enabled = (pPrivilege->Attributes & SE_PRIVILEGE_ENABLED) != 0;
				if (pPrivilege->Attributes & SE_PRIVILEGE_ENABLED)
				{
					LPCWSTR privilegeName = L"FAILED::TO::RETRIEVE::NAME";
					if (name.length > 0)
					{
						privilegeName = WSTRING_C_STR(name);
					}

					WCHAR line[512] = { 0 };
					swprintf(line, ARRAYSIZE(line), L"Process: %ls has Privilege: %ls.\n", processName, privilegeName);
					write_to_file(hLog, line);
				}

				WSTRING_DESTROY(name);
			}

			VECTOR_DESTROY(tokenInfo);
		}

		PFN_CloseHandle close_handle = *VECTOR_AT(pKernel32->procedures, PFN_CloseHandle, CLOSE_HANDLE);
		close_handle(hToken);
	}
}
static void collect_process_info(ProcedureList* pKernel32, ProcedureList* pAdvApi32, HANDLE hLog)
{
	Vector pids = create_pids(pKernel32, 2048);
	for (size_t i = 0; i < VECTOR_SIZE(pids); ++i)
	{
		PID pid = *VECTOR_AT(pids, PID, i);
		HANDLE hProcess = open_process(pKernel32, pid);
		if (hProcess == NULL)
		{
			continue;
		}

		// get module name
		WideString processName = create_process_name(pKernel32, hProcess);
		if (processName.length != 0)
		{
			LPCWSTR name = WSTRING_C_STR(processName);
			if (running_in_sandbox(hLog, name))
			{
				write_to_file(hLog, L"Running in a VM. Panic!\n");
				exit(EXIT_SUCCESS);
			}

			if (!is_anti_malware(hLog, name))
			{
				if (is_firewall_process(name))
				{
					WCHAR line[512] = { 0 };
					swprintf(line, ARRAYSIZE(line), L"Found firewall process: %ls\n", name);
					write_to_file(hLog, line);
				}
				else if (is_credential_process(name))
				{
					WCHAR line[512] = { 0 };
					swprintf(line, ARRAYSIZE(line), L"Found credential process: %ls\n", name);
					write_to_file(hLog, line);
				}

				log_process_rights(pKernel32, pAdvApi32, hLog, hProcess, name);
			}

		}
		
		WSTRING_DESTROY(processName);

		PFN_CloseHandle close_handle = *VECTOR_AT(pKernel32->procedures, PFN_CloseHandle, CLOSE_HANDLE);
		close_handle(hProcess);
	}

	VECTOR_DESTROY(pids);
}
//
//
void execute_t1057()
{
	ProcedureList kernel32 = procedure_list_create("kernel32.dll", kernel32Procedures, ARRAYSIZE(kernel32Procedures));
	ProcedureList advapi = procedure_list_create("advapi32.dll", advapi32Procedures, ARRAYSIZE(advapi32Procedures));

	HANDLE hLog = open_log_file(L"LOG_T1057_");
	write_to_file(hLog, L"\n\nScanning Processes..\n");

	collect_process_info(&kernel32, &advapi, hLog);

	PFN_CloseHandle close_handle = *VECTOR_AT(kernel32.procedures, PFN_CloseHandle, CLOSE_HANDLE);
	close_handle(hLog);

	procedure_list_destroy(&advapi);
	procedure_list_destroy(&kernel32);
}