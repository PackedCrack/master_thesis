#pragma once


#include "misc/vector.h"

typedef struct
{
	Vector procedures;	// Vector of FARPROC
} ProcedureList;

ProcedureList procedure_list_create(const char* dll, const char** ppProcedureNames, size_t numProcedures);
//ProcedureList procedure_list_create(const char* dll, Vector* pProcedures);
void procedure_list_destroy(ProcedureList* pList);