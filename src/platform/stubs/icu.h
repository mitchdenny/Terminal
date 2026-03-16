// Linux stub for <icu.h>
// Minimal ICU compatibility
#pragma once

typedef enum { U_ZERO_ERROR = 0 } UErrorCode;
#define U_FAILURE(x) ((x) > 0)
#define U_SUCCESS(x) ((x) <= 0)
