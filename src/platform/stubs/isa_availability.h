// Linux stub
#pragma once
#include <immintrin.h>
#define __ISA_AVAILABLE_SSE2 1
#define __ISA_AVAILABLE_AVX2 5
extern "C" { inline int __isa_available = __ISA_AVAILABLE_SSE2; }
