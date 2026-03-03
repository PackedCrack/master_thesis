#include "C:\\Program Files\\University of Arizona\\Tigress C Source Code Obfuscator\\Tigress\\tigress.h"
#include "T1059.001.h"

#include "../misc/common.h"
#include "../misc/function_pointers.h"
#include "../runtime_linking.h"

#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>


// Kernel32.dll
#define CREATE_PROCESS_W 0
#define CLOSE_HANDLE 1
#define WAIT_FOR_SINGLE_OBJECT 2
#define WIN_EXEC 3
static const char* t1059_kernel32Procedures[4] = { "CreateProcessW", "CloseHandle", "WaitForSingleObject", "WinExec" };
// Shell32.dll
#define SHELL_EXECUTE_W 0
static const char* t1059_shell32Procedures[1] = { "ShellExecuteW" };


static launch_ps_1(ProcedureList* pKernel32)
{
	STARTUPINFOW si = { 0 };
	PROCESS_INFORMATION pi = { 0 };

	WCHAR cmdline[] =
		L"powershell.exe -NoProfile -ExecutionPolicy Bypass -Command \"echo 'Hello PowerShell!'\"";
	PFN_CreateProcessW create_process_w = *VECTOR_AT(pKernel32->procedures, PFN_CreateProcessW, CREATE_PROCESS_W);
	if (!create_process_w(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
	{
		PRINT_WIN32_ERROR(CreateProcessW);
		assert(FALSE);
	}


	PFN_WaitForSingleObject wait_for_single_object = *VECTOR_AT(pKernel32->procedures, 
																PFN_WaitForSingleObject, 
																WAIT_FOR_SINGLE_OBJECT);
	wait_for_single_object(pi.hProcess, INFINITE);

	PFN_CloseHandle close_handle = *VECTOR_AT(pKernel32->procedures, PFN_CloseHandle, CLOSE_HANDLE);
	close_handle(pi.hThread);
	close_handle(pi.hProcess);
}
static launch_ps_2(ProcedureList* pShell32)
{
	LPCWSTR args = L"-NoProfile -ExecutionPolicy Bypass echo \"Hello PowerShell!\"; Start-Sleep -Seconds 2";

	PFN_ShellExecuteW shell_execute_w = *VECTOR_AT(pShell32->procedures, PFN_ShellExecuteW, SHELL_EXECUTE_W);
	if (shell_execute_w(NULL, L"open", L"powershell.exe", args, NULL, SW_SHOWNORMAL) <= 32)
	{
		PRINT_WIN32_ERROR(ShellExecuteW);
		assert(FALSE);
	}
}
static launch_ps_3(ProcedureList* pKernel32)
{
	PFN_WinExec win_exec = *VECTOR_AT(pKernel32->procedures, PFN_WinExec, WIN_EXEC);
	int32_t err = win_exec("powershell.exe -NoProfile -ExecutionPolicy Bypass echo \"Hello PowerShell!\"; Start-Sleep -Seconds 2",
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
	ProcedureList kernel32 = procedure_list_create("kernel32.dll", t1059_kernel32Procedures, ARRAYSIZE(t1059_kernel32Procedures));
	ProcedureList shell32 = procedure_list_create("shell32.dll", t1059_shell32Procedures, ARRAYSIZE(t1059_shell32Procedures));
	
	launch_ps_1(&kernel32);
	launch_ps_2(&shell32);
	launch_ps_3(&kernel32);
	
	procedure_list_destroy(&shell32);
	procedure_list_destroy(&kernel32);
}