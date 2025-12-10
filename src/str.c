#include "str.h"

#include <assert.h>
#include <string.h>


String details_string_create(const char* literal, size_t size)
{
    String s = { 0 };
    s.array = VECTOR_CREATE(char, size);
    s.length = size - 1;    // length does not count the null terminator

    assert(s.array.pData != NULL);
    memcpy(s.array.pData, literal, size);

    return s;
}
void details_string_destroy(String* pStr)
{
    VECTOR_DESTROY(pStr->array);
    pStr->length = 0;
}
const char* details_string_c_str(const String* pStr)
{
    return (char*) pStr->array.pData;
}
size_t details_string_size(const String* pStr)
{
    return pStr->length;
}