#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#if defined(__GNUC__) || defined(__clang__)
#define ZJSON_LIKELY(x) __builtin_expect(!!(x), 1)
#define ZJSON_UNLIKELY(x) __builtin_expect(!!(x), 0)
#define ZJSON_FORCE_INLINE __attribute__((always_inline))
#elif defined(_MSC_VER)
#define ZJSON_LIKELY(x) x
#define ZJSON_UNLIKELY(x) x
#define ZJSON_FORCE_INLINE __forceinline
#else
#define ZJSON_LIKELY(x) x
#define ZJSON_UNLIKELY(x) x
#define ZJSON_FORCE_INLINE inline
#endif

namespace zjson {

enum Type {
    JSON_NUMBER = 0,
    JSON_INT,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT,
    JSON_TRUE,
    JSON_FALSE,
    JSON_NULL = 0xF
};
enum Error {
    ERROR_NO_ERROR,
    ERROR_BAD_NUMBER,
    ERROR_BAD_STRING,
    ERROR_BAD_IDENTIFIER,
    ERROR_BAD_ROOT,
    ERROR_STACK_OVERFLOW,
    ERROR_STACK_UNDERFLOW,
    ERROR_MISMATCH_BRACKET,
    ERROR_UNEXPECTED_CHARACTER,
    ERROR_BREAKING_BAD,
    ERROR_OUT_OF_MEMORY
};

} // namespace zjson
