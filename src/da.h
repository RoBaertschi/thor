#pragma once

// ================
// ------ da ------
// da is a dynamic array, it should contain the following members:
// *items   - A pointer to a item
// count    - A integer type for the size, often a size_t
// capacity - A integer type for the capacity, often a size_t
//
// The idea is from tscoding, the implementation is by me yanked from the lua
// branch in the rob repository.
// ================

#define DA_INITIAL_CAP 4

/// Returns the new capacity

#define da_grow(da, item_size)                                                \
    do {                                                                      \
        if ((da)->items != NULL) {                                            \
            if (((da)->items = realloc(                                       \
                     (da)->items, item_size * (da)->capacity * 2)) == NULL) { \
                (da)->capacity = 0;                                           \
                (da)->count    = 0;                                           \
            } else {                                                          \
                (da)->capacity *= 2;                                          \
            }                                                                 \
        } else {                                                              \
            if (((da)->items = malloc(item_size * DA_INITIAL_CAP)) == NULL) { \
                (da)->capacity = 0;                                           \
                (da)->count    = 0;                                           \
            } else {                                                          \
                (da)->capacity = DA_INITIAL_CAP;                              \
            }                                                                 \
        }                                                                     \
    } while (0);

#define da_ensure_size(da, size, item_size) \
    while ((da)->capacity < size) {         \
        da_grow((da), item_size);           \
    }

#define da_append(da, item)                                  \
    do {                                                     \
        da_ensure_size((da), (da)->count + 1, sizeof(item)); \
        (da)->items[((da)->count)++] = item;                 \
    } while (0);

#define da_pop(da)                     \
    do {                               \
        (da)->count = (da)->count - 1; \
    } while (0);

#define da_str_append(da, ...)                             \
    do {                                                   \
        char  *items[]   = {__VA_ARGS__};                  \
        size_t item_size = sizeof(items) / sizeof(char *); \
        for (size_t i = 0; i < item_size; i++)             \
            da_append((da), items[i]);                     \
    } while (0);
