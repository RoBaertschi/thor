#include "utils.hpp"
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>

void* allocator_malloc(allocator *alloc, usz size, usz alignment, allocator_flag flag) {
    return alloc->func(alloc->allocator_data, allocator_type_alloc, size, alignment, NULL, 0, flag);
}

void* arena_allocator_func(void* allocator_data, allocator_type type, usz size, usz alignment, void* old_memory, usz old_size, allocator_flag flag) {
    arena_allocator* arena = (arena_allocator*)allocator_data;

    switch (type) {
    case allocator_type_alloc: {
        uintptr_t curr_ptr = (uintptr_t)arena->buf + (uintptr_t)arena->curr_offset;
        uintptr_t offset = align_forward(curr_ptr, alignment);
        offset -= (uintptr_t)arena->buf;

        if (offset+size <= arena->buf_len) {
            void *ptr = &arena->buf[offset];
            arena->prev_offset = offset;
            arena->curr_offset = offset+size;

            if (flag & allocator_flag_clear_to_zero) {
                memset(ptr, 0, size);
            }
            return ptr;
        }
        return NULL;
    }

    case allocator_type_realloc: {
        unsigned char *old_mem = (unsigned char*)old_memory;
        if (old_mem == NULL || old_size == 0) {
            return arena_allocator_func(arena, allocator_type_alloc, size, alignment, NULL, 0, flag);
        } else if (arena->buf <= old_mem && old_mem < arena->buf+arena->buf_len) {
            if (arena->buf+arena->prev_offset == old_mem) {
                arena->curr_offset = arena->prev_offset + size;
                if (size > old_size && allocator_flag_clear_to_zero & flag) {
                    memset(&arena->buf[arena->curr_offset], 0, size-old_size);
                }
                return old_memory;
            } else {
                void *new_memory = arena_allocator_func(arena, allocator_type_alloc, size, alignment, NULL, 0, flag);
                usz copy_size = old_size < size ? old_size : size;
                memmove(new_memory, old_mem, copy_size);
                return new_memory;
            }
        } else {
            assert(0 && "Memory is out of bounds of the buffer in this arena");
            return NULL;
        }
    }
    case allocator_type_free:
        return NULL;
    case allocator_type_free_all:
        arena->curr_offset = 0;
        arena->prev_offset = 0;
        return NULL;
    }

    return NULL;
}

void arena_init(arena_allocator* arena, void *backing_buffer, usz backing_buffer_length) {
    arena->buf = (unsigned char*)backing_buffer;
    arena->buf_len = backing_buffer_length;
    arena->curr_offset = 0;
    arena->prev_offset = 0;
}

uintptr_t align_forward(uintptr_t ptr, usz align) {
    uintptr_t p, a, modulo;

    assert(is_power_of_two(align));

    p = ptr;
    a = (uintptr_t)align;
    modulo = p & (a-1);

    if (modulo != 0) {
        p += a - modulo;
    }
    return p;
}


void str_buffer_append(str_buffer *buf, str str) {
    
}
void str_buffer_appendz(str_buffer *buf, izstr str) {}
