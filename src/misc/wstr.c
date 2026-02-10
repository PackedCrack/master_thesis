#include "C:\\Program Files\\University of Arizona\\Tigress C Source Code Obfuscator\\Tigress\\tigress.h"
#include "wstr.h"

#include <assert.h>
#include <string.h>


WideString details_wstring_create(const wchar_t* literal, size_t size)
{
    WideString s = { 0 };
    s.array = VECTOR_CREATE(wchar_t, size);
    s.length = size - 1;    // length does not count the null terminator

    assert(s.array.pData != NULL);
    memcpy(s.array.pData, literal, size * sizeof(wchar_t));

    return s;
}
WideString details_wstring_create2(const wchar_t* pStr)
{
    WideString s = { 0 };
    s.length = wcslen(pStr);
    s.array = VECTOR_CREATE(wchar_t, s.length + 1);

    assert(s.array.pData != NULL);

    if (s.length)
    {
        memcpy(s.array.pData, pStr, s.length * sizeof(wchar_t));
    }

    wchar_t* pNull = (wchar_t*) s.array.pData;
    pNull[s.length] = L'\0';

    return s;
}
void details_wstring_destroy(WideString* pStr)
{
    VECTOR_DESTROY(pStr->array);
    pStr->length = 0;
}
const wchar_t* details_wstring_c_str(const WideString* pStr)
{
    return (wchar_t*) pStr->array.pData;
}
size_t details_wstring_size(const WideString* pStr)
{
    return pStr->length;
}
wchar_t details_wstring_back(const WideString* pStr)
{
    assert(pStr->array.pData != NULL);
    assert(pStr->length > 0);

    wchar_t* string = pStr->array.pData;
    return string[pStr->length - 1];
}
void details_wstring_push_back(WideString* pStr, wchar_t c)
{
    assert(pStr->array.pData != NULL);

    wchar_t newElement = L'\0';
    VECTOR_PUSH_BACK(pStr->array, wchar_t, newElement);

    wchar_t* string = pStr->array.pData;
    string[pStr->length] = c;
    pStr->length += 1;
}
WideString details_wstring_concatenate(WideString* pLhs, const wchar_t* pRhs)
{
    assert(pLhs->array.pData != NULL);
    assert(pRhs != NULL);

    WideString concatenated = { 0 };
    concatenated.length = pLhs->length + wcslen(pRhs);
    concatenated.array = VECTOR_CREATE(wchar_t, concatenated.length + 1);

    wchar_t* pDst = (wchar_t*) concatenated.array.pData;
    const wchar_t* pSrc = (wchar_t*) pLhs->array.pData;
    memcpy(pDst, pSrc, pLhs->length * sizeof(wchar_t));
    pDst += pLhs->length;
    
    memcpy(pDst, pRhs, wcslen(pRhs) * sizeof(wchar_t));

    wchar_t* pElement = VECTOR_BACK(concatenated.array, wchar_t);
    *pElement = L'\0';

    return concatenated;
}