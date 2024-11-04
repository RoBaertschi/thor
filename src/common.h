#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>

#ifndef NORETURN
#ifdef __GNUC__
#define NORETURN __attribute__((noreturn))
#else
#define NORETURN
#endif
#endif

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

typedef struct str str;
struct str {
    char *ptr;
    usz   len;
};

// This allocates the string in the string pool. So if we forgot to free or
// don't want to, it will not leak.
char *to_cstr(str str);
// Converts a String Literal or Normal String to a str.
str  to_str(const char *s);
str  to_strl(const char *s, usz len);
str  str_clone(str s);
void str_destroy(str s);

bool                                          str_equal(str s1, str s2);
str __attribute__((__format__(printf, 1, 2))) str_format(const char *format,
                                                         ...);
str str_format_va(const char *format, va_list va);
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
