#pragma once


#include "vector.h"

typedef struct
{
	Vector procedures;	// Vector of FARPROC
} ProcedureList;

ProcedureList procedure_list_create(const char* dll, const Vector* pProcedures);
void procedure_list_destroy(ProcedureList* pList);