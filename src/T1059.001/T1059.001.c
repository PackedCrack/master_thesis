#include "T1059.001.h"

#include "../misc/common.h"
#include "../runtime_linking.h"

#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#define WIN_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>


// Kernel32.dll
#define CREATE_PROCESS_W 0
#define CLOSE_HANDLE 1
#define WAIT_FOR_SINGLE_OBJECT 2
#define WIN_EXEC 3
static const char* kernel32Procedures[4] = { "CreateProcessW", "CloseHandle", "WaitForSingleObject", "WinExec" };
// Shell32.dll
#define SHELL_EXECUTE_W 0
static const char* shell32Procedures[1] = { "ShellExecuteW" };


static LPCWSTR ps = L"powershell.exe -NoProfile -ExecutionPolicy Bypass echo \"Hello PowerShell!\"";


static launch_ps_1(ProcedureList* pKernel32)
{
	STARTUPINFOW si = { 0 };
	PROCESS_INFORMATION pi = { 0 };
	FARPROC PFN_CreateProcessW = *VECTOR_AT(pKernel32->procedures, FARPROC, CREATE_PROCESS_W);
	if (!PFN_CreateProcessW(NULL, ps, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
	{
		PRINT_WIN32_ERROR(CreateProcessW);
		assert(FALSE);
	}


	FARPROC PFN_WaitForSingleObject = *VECTOR_AT(pKernel32->procedures, FARPROC, WAIT_FOR_SINGLE_OBJECT);
	PFN_WaitForSingleObject(pi.hProcess, INFINITE);

	FARPROC PFN_CloseHandle = *VECTOR_AT(pKernel32->procedures, FARPROC, CLOSE_HANDLE);
	PFN_CloseHandle(pi.hThread);
	PFN_CloseHandle(pi.hProcess);
}
static launch_ps_2(ProcedureList* pShell32)
{
	LPCWSTR args = L"-NoProfile -ExecutionPolicy Bypass echo \"Hello PowerShell!\"; Start-Sleep -Seconds 2";

	FARPROC PFN_ShellExecuteW = *VECTOR_AT(pShell32->procedures, FARPROC, SHELL_EXECUTE_W);
	if (PFN_ShellExecuteW(NULL, L"open", L"powershell.exe", args, NULL, SW_SHOWNORMAL) <= 32)
	{
		PRINT_WIN32_ERROR(ShellExecuteW);
		assert(FALSE);
	}
}
static launch_ps_3(ProcedureList* pKernel32)
{
	FARPROC PFN_WinExec = *VECTOR_AT(pKernel32->procedures, FARPROC, WIN_EXEC);
	int32_t err = PFN_WinExec("powershell.exe -NoProfile -ExecutionPolicy Bypass echo \"Hello PowerShell!\"; Start-Sleep -Seconds 2", 
							  SW_SHOWNORMAL);
	if (err <= 31)
	{
		printf("WinExec failed with error: 0x%lX", err);
		assert(FALSE);
	}
}
//
//
void execute_t1059_001()
{
	ProcedureList kernel32 = procedure_list_create("kernel32.dll", kernel32Procedures, ARRAYSIZE(kernel32Procedures));
	ProcedureList shell32 = procedure_list_create("shell32.dll", shell32Procedures, ARRAYSIZE(shell32Procedures));
	
	launch_ps_1(&kernel32);
	launch_ps_2(&shell32);
	launch_ps_3(&kernel32);
	
	procedure_list_destroy(&shell32);
	procedure_list_destroy(&kernel32);
}