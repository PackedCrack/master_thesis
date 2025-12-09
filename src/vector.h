#pragma once

#include <assert.h>
#include <stdint.h>


typedef struct
{
	void* pData;
	size_t size;
	size_t capacity;
	size_t typeSize;
} Vector;

Vector details_vector_create(size_t typeSize, size_t numElements);
#define VECTOR_CREATE(T, count) details_vector_create(sizeof(T), count)

void details_vector_destroy(Vector* pVector);
#define VECTOR_DESTROY(vector) details_vector_destroy(&vector)

void* details_vector_at(Vector* pVector, size_t index);
#define VECTOR_AT(vector, T, index) (T*) details_vector_at(&vector, index)

void* details_vector_front(Vector* pVector);
#define VECTOR_FRONT(vector, T) (T*) details_vector_front(&vector)

void* details_vector_back(Vector* pVector);
#define VECTOR_BACK(vector, T) (T*) details_vector_back(&vector)

int32_t details_vector_empty(Vector* pVector);
#define VECTOR_EMPTY(vector) details_vector_empty(&vector)

size_t details_vector_size(Vector* pVector);
#define VECTOR_SIZE(vector) details_vector_size(&vector)

size_t details_vector_capacity(Vector* pVector);
#define VECTOR_CAPACITY(vector) details_vector_capacity(&vector)

void details_vector_push_back(Vector* pVector, void* pNewElement);
#define VECTOR_PUSH_BACK(vector, object) assert(sizeof(object) == vector.typeSize); details_vector_push_back(&vector, &object)

void details_vector_pop_back(Vector* pVector);
#define VECTOR_POP_BACK(vector) details_vector_pop_back(&vector)

void details_vector_swap(Vector* pVector, size_t index1, size_t index2);
#define VECTOR_SWAP(vector, index1, index2) details_vector_pop_back(&vector, index1, index2)

