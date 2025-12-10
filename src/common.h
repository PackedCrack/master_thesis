#pragma once

#include <stdint.h>


void print_win32_err(const char* func);
#define PRINT_WIN32_ERROR(callable) print_win32_err(#callable);