#pragma once

#include "vector.h"


typedef struct
{
	Vector array;
	size_t length;
} String;

String details_string_create(const char* literal, size_t size);
#define STRING_CREATE(literal) details_string_create(literal, sizeof(literal))

void details_string_destroy(String* pStr);
#define STRING_DESTROY(string) details_string_destroy(&string)

const char* details_string_c_str(const String* pStr);
#define STRING_C_STR(string) details_string_c_str(&string)

size_t details_string_size(const String* pStr);
#define STRING_SIZE(string) details_string_size(&string)