#pragma once
// Linux stub for strsafe.h
#include <cstdio>
#include <cstring>
#define StringCchCopyW(d, n, s) wcsncpy(d, s, n)
#define StringCchCatW(d, n, s) wcsncat(d, s, n)
#define StringCchPrintfW(d, n, f, ...) swprintf(d, n, f, __VA_ARGS__)
