#include "vector.h"


int main(int argc, char** argv)
{
	Vector ints = VECTOR_CREATE(int32_t, 3);

	for (int32_t i = 0; i < 16; ++i)
	{
		VECTOR_PUSH_BACK(ints, i);
	}


	VECTOR_DESTROY(ints);

	return 0;
}