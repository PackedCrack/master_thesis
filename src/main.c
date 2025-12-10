#include "vector.h"
#include "str.h"
#include "runtime_linking.h"

#include "stdio.h"

#ifndef NDEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

int main(int argc, char** argv)
{
#ifndef NDEBUG
	_CrtSetDbgFlag(
		_CRTDBG_ALLOC_MEM_DF |
		_CRTDBG_LEAK_CHECK_DF 
	);
#endif


	Vector procedures = VECTOR_CREATE(String, 0);
	String p1 = STRING_CREATE("VirtualProtect");
	VECTOR_PUSH_BACK(procedures, String, p1);
	String p2 = STRING_CREATE("VirtualFree");
	VECTOR_PUSH_BACK(procedures, String, p2);
	String p3 = STRING_CREATE("FindFirstFileA");
	VECTOR_PUSH_BACK(procedures, String, p3);

	ProcedureList pl = procedure_list_create("kernel32.dll", &procedures);
	procedure_list_destroy(&pl);

	for (size_t i = 0u; i < VECTOR_SIZE(procedures); ++i)
	{
		String* pProcedure = VECTOR_AT(procedures, String, i);
		STRING_DESTROY(*pProcedure);
	}

	VECTOR_DESTROY(procedures);

	return 0;
}