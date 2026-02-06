#include "misc/common.h"
#include "misc/vector.h"
#include "misc/str.h"
#include "runtime_linking.h"

#include "T1082/T1082.h"
#include "T1083/T1083.h"
#include "T1057/T1057.h"
#include "T1070.004/T1070.004.h"
#include "T1547.001/T1547.001.h"
#include "T1059.001/T1059.001.h"
#include "T1005/T1005.h"

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
	init_common();

	execute_t1070_004(argv);

	execute_t1082();
	
	execute_t1083();

	execute_t1057();

	execute_t1574_001(argv);
	
	execute_t1059_001();

	execute_t1005();

	deinit_common();

	return 0;
}