#pragma once

#include <stdint.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>


void print_win32_err(const char* func);
#define PRINT_WIN32_ERROR(callable) print_win32_err(#callable);

HANDLE open_log_file(LPWSTR basename);