#include "misc/vector.h"
#include "misc/str.h"
#include "runtime_linking.h"

#include "T1082/T1082.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>


#ifndef NDEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

static set_seed()
{
	srand((unsigned) time(NULL));
}
//
//
int main(int argc, char** argv)
{
#ifndef NDEBUG
	_CrtSetDbgFlag(
		_CRTDBG_ALLOC_MEM_DF |
		_CRTDBG_LEAK_CHECK_DF 
	);
#endif

	set_seed();

	execute_t1082();

	return 0;
}