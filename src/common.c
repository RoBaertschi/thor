#include "common.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct string_pool_node string_pool_node;
struct string_pool_node {
    string_pool_node *next;
    char             *data;
};

static struct {
    string_pool_node *head;
} string_pool = {.head = NULL};

char *to_cstr_in_string_pool(str str) {
    char *new_str = malloc(str.len + 1);
    memcpy(new_str, str.ptr, str.len + 1);
    new_str[str.len] = '\0';

    string_pool_take_ownership(new_str);
    return new_str;
}

char *to_cstr(str str) {
    char *new_str = malloc(str.len + 1);
    memcpy(new_str, str.ptr, str.len + 1);
    new_str[str.len] = '\0';
    return new_str;
}

str to_str(char const *s) {
    usz   len     = strlen(s);
    char *new_str = malloc(len);

    assert(new_str != NULL && "to_str could not alloc");

    memcpy(new_str, s, len);

    return (str){
        .ptr = new_str,
        .len = len,
    };
}

str to_strl(char const *s, usz len) {
    char *new_str = malloc(len);

    memcpy(new_str, s, len);

    return (str){
        .ptr = new_str,
        .len = len,
    };
}

str str_clone(str s) {
    str new_str = {
        .len = s.len,
        .ptr = malloc(s.len),
    };

    if (new_str.ptr == NULL) {
        assert(0 && "Failed to malloc for a str_clone, this indicates that the "
                    "system has run out of memory.");
        return (str){.len = 0, .ptr = NULL};
    }

    memcpy(new_str.ptr, s.ptr, s.len);

    return new_str;
}

void str_destroy(str s) { free(s.ptr); }

bool str_equal(str s1, str s2) {
    if (s1.len != s2.len) {
        return false;
    }

    // s1.len == s2.len
    for (usz i = 0; i < s1.len; i++) {
        if (s1.ptr[i] != s2.ptr[i]) {
            return false;
        }
    }

    return true;
}

str __attribute__((__format__(printf, 1, 2))) str_format(char const *format,
                                                         ...) {
    va_list va;
    va_start(va, format);
    str str = str_format_va(format, va);
    va_end(va);
    return str;
}

str str_format_va(char const *format, va_list va) {
    va_list va1;
    va_copy(va1, va);
    usz needed = vsnprintf(NULL, 0, format, va1) + 1;
    va_end(va1);
    char *buffer = malloc(needed);
    vsnprintf(buffer, needed, format, va);
    char *buffer_without_0 = malloc(needed - 1);
    memcpy(buffer_without_0, buffer, needed - 1);
    free(buffer);
    return (str){.len = needed - 1, .ptr = buffer_without_0};
}

void str_fprint(FILE *file, str to_print) {
    fwrite(to_print.ptr, sizeof(char), to_print.len, file);
}

void str_fprintln(FILE *file, str to_print) {
    str_fprint(file, to_print);
    fwrite("\n", sizeof(char), 1, file);
}

void string_pool_free(char *str) {
    string_pool_node *node      = string_pool.head;
    string_pool_node *prev_node = NULL;

    while (node != NULL) {
        if (str == node->data) {
            free(node->data);
            if (prev_node != NULL) {
                prev_node->next = node->next;
            } else {
                string_pool.head = NULL;
            }
            free(node);
            return;
        }

        prev_node = node;
        node      = node->next;
    }

    // TODO: Maybe warn about missing string
}
void string_pool_take_ownership(char *str) {
    string_pool_node *new_node = malloc(sizeof(string_pool_node));
    new_node->data             = str;
    new_node->next             = string_pool.head;
    string_pool.head           = new_node->next;
}
void string_pool_free_all(void) {
    string_pool_node *node = string_pool.head;

    while (node != NULL) {
        string_pool_node *old_node = node;
        node                       = node->next;

        free(old_node->data);
        free(old_node);
    }

    string_pool.head = NULL;
}

bool vec_ensure_size(usz len, usz *cap, void **ptr, usz item_size,
                     usz items_to_add) {

    if (*ptr == NULL && *cap == 0) {
        usz new_cap = (items_to_add <= 4 ? 4 : items_to_add);
        *ptr        = malloc(new_cap * item_size);
        if (ptr == NULL) {
            return false;
        }
        *cap = new_cap;
        return true;
    }

    while (len + items_to_add > *cap) {
        void *new_ptr = realloc(*ptr, *cap * 2 * item_size);
        if (new_ptr == NULL) {
            return false;
        }
        *ptr = new_ptr;
        *cap = *cap * 2;
    }
    return true;
}
