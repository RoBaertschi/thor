#pragma once

#include <cstddef>
#include <cstdint>

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
#define OS_WINDOWS 1
#elif defined(__linux__)
#define OS_LINUX 1
#elif defined(__unix__) || defined(__unix)
#define OS_UNIX 1
#elif defined(__APPLE__) && defined(__MACH__)
#define OS_OSX 1
#else
#error "Unsupported OS"
#endif

#if defined(__clang__)
#define COMPILER_CLANG 1
#elif defined(__GNUC__)
#define COMPILER_GCC 1
#elif defined(_MSC_VER)
#define COMPILER_MSVC 1
#elif defined(__MINGW32__) || defined(__MINGW64__)
#define COMPILER_MINGW 1
#else
#warning                                                                       \
    "Unsupported Compiler, there is currently no compiler support required but this could break in the future"
#endif

#define BIT(x) (1<<(x))

// Unsigned Integers
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

// Signed Integers
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef size_t usz;

// Floats
typedef float f32;
static_assert(sizeof(f32) == 4, "A float has to be 4 bytes");
typedef double f64;
static_assert(sizeof(f64) == 8, "A double has to be 8 bytes");

/// Allocator

#define DEFAULT_ALIGNMENT (2*sizeof(void*))

inline bool is_power_of_two(uintptr_t x) {
    return (x & (x-1)) == 0;
}

uintptr_t align_forward(uintptr_t ptr, usz align);

typedef enum allocator_type {
    allocator_type_alloc,
    allocator_type_realloc,
    allocator_type_free,
    allocator_type_free_all,
} allocator_type;

typedef enum allocator_flag {
    allocator_flag_none             = 0,
    allocator_flag_clear_to_zero    = BIT(0),
} allocator_flag;

typedef void*(*allocator_func)(void* allocator_data, allocator_type type, usz size, usz alignment, void* old_memory, usz old_size, allocator_flag flag);
typedef struct allocator allocator;
struct allocator {
    allocator_func  func;
    void*           allocator_data;
};

void* allocator_malloc(allocator *alloc, usz size, usz alignment = DEFAULT_ALIGNMENT, allocator_flag flag = allocator_flag_clear_to_zero);

typedef struct arena_allocator arena_allocator;
struct arena_allocator {
    unsigned char*  buf;
    usz             buf_len;
    usz             prev_offset;
    usz             curr_offset;
};

void* arena_allocator_func(void* allocator_data, allocator_type type, usz size, usz alignment, void* old_memory, usz old_size, allocator_flag flag);
void arena_init(arena_allocator* arena, void *backing_buffer, usz backing_buffer_length);
inline void arena_to_allocator(arena_allocator *a, allocator* alloc) {
    alloc->allocator_data = a;
    alloc->func = arena_allocator_func;
}

/// Strings

// Const always modifies the element BEFORE it, EXCEPT if it is the first, then
// it will modify the one after it char const *     == const char * char * const
// != char const *

// Zero determinated string
typedef char *zstr;
// Immutable zero determinated string
typedef char const *izstr;
// Non zero determinated string with a length
typedef struct str str;
struct str {
  usz len;
  char *ptr;
};

typedef struct str_buffer str_buffer;
struct str_buffer {
  usz len;
  usz cap;
  char *buf;
};

void str_buffer_append(str_buffer *buf, str str);
void str_buffer_appendz(str_buffer *buf, izstr str);
