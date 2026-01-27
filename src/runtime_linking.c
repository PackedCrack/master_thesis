#include "runtime_linking.h"

#include "misc/common.h"
#include "misc/str.h"

#include <assert.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static HMODULE dll_module_handle(const char* dll)
{
	HMODULE module = LoadLibraryA(dll);
	if (module == NULL)
	{
		PRINT_WIN32_ERROR(LoadLibraryA);
	}

	return module;
}
ProcedureList procedure_list_create(const char* dll, const char** ppProcedureNames, size_t numProcedures)
{
	assert(dll != NULL);
	assert(ppProcedureNames != NULL);

	ProcedureList procs = { 0 };
	procs.procedures = VECTOR_CREATE(FARPROC, 0);

	HMODULE module = dll_module_handle(dll);
	for (size_t i = 0u; i < numProcedures; ++i)
	{
		const char* pName = ppProcedureNames[i];
		FARPROC pProcedure = GetProcAddress(module, pName);
		if (pProcedure == NULL)
		{
			PRINT_WIN32_ERROR(GetProcAddress);
		}

		VECTOR_PUSH_BACK(procs.procedures, FARPROC, pProcedure);
	}

	return procs;
}
//ProcedureList procedure_list_create(const char* dll, Vector* pProcedures)
//{
//	assert(dll != NULL);
//	assert(VECTOR_EMPTY(*pProcedures) == FALSE);
//
//	ProcedureList procs = { 0 };
//	procs.procedures = VECTOR_CREATE(FARPROC, 0);
//
//	HMODULE module = dll_module_handle(dll);
//	for (size_t i = 0u; i < VECTOR_SIZE(*pProcedures); ++i)
//	{
//		String* pName = VECTOR_AT(*pProcedures, String, i);
//		FARPROC pProcedure = GetProcAddress(module, STRING_C_STR(*pName));
//		if (pProcedure == NULL)
//		{
//			PRINT_WIN32_ERROR(GetProcAddress);
//		}
//
//		VECTOR_PUSH_BACK(procs.procedures, FARPROC, pProcedure);
//	}
//
//	return procs;
//}
void procedure_list_destroy(ProcedureList* pList)
{
	VECTOR_DESTROY(pList->procedures);
}