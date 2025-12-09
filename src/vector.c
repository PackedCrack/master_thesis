#include "vector.h"


#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdio.h>


static size_t round_up_to_pow_2(size_t value)
{
	if (value <= 1)
	{
		return 1;
	}

	double exponent = ceil(log2((double) value));
	return (size_t) pow(2.0, exponent);
}
static void resize(Vector* pVector)
{
	Vector vec = details_vector_create(pVector->typeSize, details_vector_size(pVector));

	// This assumes that the elements are trivially copyable
	size_t bytesInUse = details_vector_size(pVector) * pVector->typeSize;
	memcpy(vec.pData, pVector->pData, bytesInUse);

	details_vector_destroy(pVector);
	*pVector = vec;
}
static int32_t requires_resize(Vector* pVector)
{
	return (details_vector_size(pVector) >= details_vector_capacity(pVector));
}
//
//
Vector details_vector_create(size_t typeSize, size_t numElements)
{
	assert(typeSize > 0);
	assert(numElements > 0);

	Vector vec = { 0 };
	vec.typeSize = typeSize;
	vec.size = numElements;
	vec.capacity = round_up_to_pow_2(numElements);

	size_t bytesRequired = vec.typeSize * vec.capacity;
	vec.pData = malloc(bytesRequired);
	if (vec.pData == NULL)
	{
		printf("Failed to allocate memory when creating a vector");
	}
	else
	{
		vec.pData = memset(vec.pData, 0, bytesRequired);
	}

	return vec;
}
void details_vector_destroy(Vector* pVector)
{
	assert(pVector != NULL);
	assert(pVector->pData != NULL);
	
	free(pVector->pData);
	pVector->pData = NULL;
	pVector->capacity = 0;
	pVector->size = 0;
	pVector->typeSize = 0;
}
void* details_vector_at(Vector* pVector, size_t index)
{
	assert(pVector != NULL);
	assert(pVector->pData != NULL);
	assert(pVector->size > index);

	ptrdiff_t offset = pVector->typeSize * index;
	return (uint8_t*) pVector->pData + offset;
}
void* details_vector_front(Vector* pVector)
{
	assert(pVector != NULL);
	assert(pVector->pData != NULL);
	assert(pVector->size > 0);

	return pVector->pData;
}
void* details_vector_back(Vector* pVector)
{
	assert(pVector != NULL);
	assert(pVector->pData != NULL);
	assert(pVector->size > 0);

	ptrdiff_t offset = pVector->typeSize * pVector->size;
	return (uint8_t*) pVector->pData + offset;
}
int32_t details_vector_empty(Vector* pVector)
{
	assert(pVector != NULL);
	assert(pVector->pData != NULL);

	if (pVector->size == 0)
	{
		return 1;
	}

	return 0;
}
size_t details_vector_size(Vector* pVector)
{
	assert(pVector != NULL);
	assert(pVector->pData != NULL);

	return pVector->size;
}
size_t details_vector_capacity(Vector* pVector)
{
	assert(pVector != NULL);
	assert(pVector->pData != NULL);

	return pVector->capacity;
}
void details_vector_push_back(Vector* pVector, void* pNewElement)
{
	assert(pVector != NULL);
	assert(pVector->pData != NULL);

	size_t index = details_vector_size(pVector);
	++pVector->size;

	if (requires_resize(pVector))
	{
		resize(pVector);
	}

	// This assumes a trivially copyable value_type
	void* pDst = details_vector_at(pVector, index);
	memcpy(pDst, pNewElement, pVector->typeSize);
}
void details_vector_pop_back(Vector* pVector)
{
	assert(pVector != NULL);
	assert(pVector->pData != NULL);
	assert(pVector->size > 0);

	void* pDst = details_vector_back(pVector);
	memset(pDst, 0, pVector->typeSize);
	--pVector->size;
}
void details_vector_swap(Vector* pVector, size_t index1, size_t index2)
{
	assert(pVector != NULL);
	assert(pVector->pData != NULL);
	assert(index1 < pVector->size);
	assert(index2 < pVector->size);

	
	void* pFirst = details_vector_at(pVector, index1);
	void* pSecond = details_vector_at(pVector, index2);
	
	assert(pVector->typeSize <= 256);
	uint8_t tmp[256] = { 0 };

	// This assumes a trivially copyable value_type
	memcpy(tmp, pFirst, pVector->typeSize);
	memcpy(pFirst, pSecond, pVector->typeSize);
	memcpy(pSecond, tmp, pVector->typeSize);

	assert(0);
}