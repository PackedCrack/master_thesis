#include "vector.h"


int main(int argc, char** argv)
{
	Vector ints = VECTOR_CREATE(int32_t, 1);

	for (int32_t i = 0; i < 16; ++i)
	{
		VECTOR_PUSH_BACK(ints, int32_t, i);

		if (i % 3 == 0)
		{
			VECTOR_POP_BACK(ints);
		}
	}


	VECTOR_SWAP(ints, 2, 5);
	VECTOR_SWAP(ints, 5, 2);
	VECTOR_SWAP(ints, 0, 3);

	VECTOR_SWAP_AND_POP(ints, 5);


	VECTOR_DESTROY(ints);

	return 0;
}