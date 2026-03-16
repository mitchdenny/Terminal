// Linux stub for <icu.h>
// Minimal ICU compatibility
#pragma once

typedef enum { U_ZERO_ERROR = 0 } UErrorCode;
#define U_FAILURE(x) ((x) > 0)
#define U_SUCCESS(x) ((x) <= 0)

// Stub types for UTextAdapter.h
struct UText { int dummy; };
struct URegularExpression { int dummy; };
inline void utext_close(UText*) {}
inline void uregex_close(URegularExpression*) {}
inline URegularExpression* uregex_open(const char16_t*, int32_t, uint32_t, void*, UErrorCode*) { return nullptr; }
inline void uregex_setTimeLimit(URegularExpression*, int32_t, UErrorCode*) {}
inline void uregex_setStackLimit(URegularExpression*, int32_t, UErrorCode*) {}
inline URegularExpression* uregex_clone(URegularExpression*, UErrorCode*) { return nullptr; }

#define UREGEX_CASE_INSENSITIVE 2
#define UREGEX_MULTILINE 8
#define UREGEX_LITERAL 16

inline void uregex_setUText(URegularExpression*, UText*, UErrorCode*) {}
inline int uregex_find(URegularExpression*, int, UErrorCode*) { return 0; }
inline int uregex_findNext(URegularExpression*, UErrorCode*) { return 0; }
