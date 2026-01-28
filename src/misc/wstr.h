#pragma once

#include "vector.h"


typedef struct
{
	Vector array;
	size_t length;
} WideString;

WideString details_wstring_create(const wchar_t* literal, size_t size);
#define WSTRING_CREATE(literal) details_wstring_create(literal, sizeof(literal) / sizeof(wchar_t))

WideString details_wstring_create2(const wchar_t* pStr);
#define WSTRING_CREATE_FROM_LPCWSTR(string) details_wstring_create2(string);

void details_wstring_destroy(WideString* pStr);
#define WSTRING_DESTROY(string) details_wstring_destroy(&string)

const wchar_t* details_wstring_c_str(const WideString* pStr);
#define WSTRING_C_STR(string) details_wstring_c_str(&string)

size_t details_wstring_size(const WideString* pStr);
#define WSTRING_SIZE(string) details_wstring_size(&string)

wchar_t details_wstring_back(const WideString* pStr);
#define WSTRING_BACK(string) details_wstring_back(&string)

void details_wstring_push_back(WideString* pStr, wchar_t c);
#define WSTRING_PUSH_BACK(string, c) details_wstring_push_back(&string, c);

WideString details_wstring_concatenate(WideString* pLhs, const wchar_t* pRhs);
#define WSTRING_CONCAT(string, LPCWSTR) details_wstring_concatenate(&string, LPCWSTR)