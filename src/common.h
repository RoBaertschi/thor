#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "uthash.h"

#ifndef NORETURN
#ifdef __GNUC__
#define NORETURN __attribute__((noreturn))
#else
#define NORETURN
#endif
#endif

#define UNREACHABLE(why) log_fatal("UNREACHABLE reached: " why);

typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;
typedef size_t    usz;
typedef uintptr_t uptr;

typedef int8_t    i8;
typedef int16_t   i16;
typedef int32_t   i32;
typedef int64_t   i64;
typedef ptrdiff_t isz;

#define HASH_FIND_USZ(head, findusz, out) \
    HASH_FIND(hh, head, findusz, sizeof(usz), out)
#define HASH_ADD_USZ(head, uszfield, add) \
    HASH_ADD(hh, head, uszfield, sizeof(usz), add)
#define HASH_REPLACE_USZ(head, uszfield, add, replaced) \
    HASH_REPLACE(hh, head, uszfield, sizeof(usz), add, replaced)

// NOTE: str's are completly immutable. Every mutation should make a copy, this
// will allow for just coping the struct around without having to copy the whole
// string.
typedef struct str str;
struct str {
    char *ptr;
    usz   len;
};

// This allocates the string in the string pool. So if we forgot to free or
// don't want to, it will not leak.
char *to_cstr_in_string_pool(str str);
// You will have to call free() on the result.
char *to_cstr(str str);
// Converts a String Literal or Normal String to a str.
str   to_str(char const *s);
str   to_strl(char const *s, usz len);
str   str_clone(str s);
void  str_destroy(str s);

bool  str_equal(str s1, str s2);
str   str_format(char const *format, ...)
    __attribute__((__format__(printf, 1, 2)));
str  str_format_va(char const *format, va_list va);
// Prints the provided str to file
void str_fprint(FILE *file, str to_print);
void str_fprintln(FILE *file, str to_print);

void string_pool_free(char *str);
// This function takes the ownership of a string.
// You can still use the pointer after giving it to this function, it just has
// to be freed with the string pool;
void string_pool_take_ownership(char *str);
void string_pool_free_all(void);

bool vec_ensure_size(usz len, usz *cap, void **ptr, usz item_size,
                     usz items_to_add);

#define DEBUG "\033[90m"
#define WARNING "\033[93m"
#define ERROR "\033[91m"
#define FATAL "\033[91m"

#define RESET "\033[0m"

void log_register_file(FILE *file);
void log_debug(char const *format, ...);
void log_info(char const *format, ...);
void log_warning(char const *format, ...);
void log_error(char const *format, ...);
// Aborts
void log_fatal(char const *format, ...) NORETURN;
