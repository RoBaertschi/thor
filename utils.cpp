#include "utils.hpp"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

usz calc_padding_with_header(uptr ptr, uptr alignment, usz header_size) {
    uptr p, a, modulo, padding, needed_space;

    assert(is_power_of_two(alignment));

    p = ptr;
    a = alignment;
    modulo = p & (a-1);

    padding = 0;
    needed_space = 0;

    if (modulo != 0) {
        padding = a - modulo;
    }

    needed_space = (uptr)header_size;

    if (padding < needed_space) {
        needed_space -= padding;
        if ((needed_space & (a-1)) != 0) {
            padding += a * (1+(needed_space/a));
        } else {
            padding += a * (needed_space/a);
        }
    }

    return (usz)padding;
}

free_list_node *free_list_find_first(free_list_allocator *fl, usz size, usz alignment, usz  *padding_, free_list_node **prev_node_) {
    free_list_node *node = fl->head;
    free_list_node *prev_node = NULL;

    usz padding = 0;

    while (node != NULL) {
        padding = calc_padding_with_header((uptr)node, (uptr)alignment, sizeof(free_list_allocation_header));
        usz required_space = size + padding;
        if (node->block_size >= required_space) {
            break;
        }
        prev_node = node;
        node = node->next;
    }

    if (padding_) {
        *padding_ = padding;
    }
    if (prev_node_) {
        *prev_node_ = prev_node;
    }
    return node;
}

free_list_node *free_list_find_best(free_list_allocator *fl, usz size, usz alignment, usz *padding_, free_list_node **prev_node_) {
    usz smallest_diff = ~(usz)0;

    free_list_node *node = fl->head;
    free_list_node *prev_node = NULL;
    free_list_node *best_node = NULL;

    usz padding = 0;
    while (node != NULL) {
        padding = calc_padding_with_header((uptr)node, (uptr)alignment, sizeof(free_list_allocation_header));
        usz required_space = size + padding;
        if (node->block_size >= required_space && (node->block_size - required_space < smallest_diff)) {
            best_node = node;
            smallest_diff = node->block_size - required_space;
        }
        prev_node = node;
        node = node->next;
    }

    if (padding_) {
        *padding_ = padding;
    }
    if (prev_node_) {
        *prev_node_ = prev_node;
    }
    return node;
}

void *free_list_allocator_func (void* allocator_data, allocator_type type, usz size, usz alignment, void* old_memory, usz old_size, allocator_flag flag) {
    free_list_allocator *fl = (free_list_allocator *)allocator_data;

    switch (type) {

    case allocator_type_alloc:
    case allocator_type_realloc:
    case allocator_type_free:
    case allocator_type_free_all: {
            fl->used = 0;
            free_list_node *first_node = (free_list_node *)fl->data;
            first_node->block_size = fl->size;
            first_node->next = NULL;
            fl->head = first_node;

            return NULL;
        }
    }
    return NULL;
}

void free_list_init(free_list_allocator *fl, void *data, size_t size) {
    fl->data = data;
    fl->size = size;

}

void pool_init(pool_allocator *p, void *backing_buffer, usz backing_buffer_length, usz chunk_size, usz chunk_alignment) {
    uptr initial_start = (uptr)backing_buffer;
    uptr start = align_forward(initial_start, chunk_alignment);
    backing_buffer_length -= (usz)(start-initial_start);

    chunk_size = align_forward(chunk_size, chunk_alignment);

    assert(chunk_size >= sizeof(pool_free_node) && "Chunk size is too small");
    assert(backing_buffer_length >= chunk_size && "Backing buffer lenght is smaller then chunk size");

    p->buf = (unsigned char *)backing_buffer;
    p->buf_len = backing_buffer_length;
    p->chunk_size = chunk_size;
    p->head = NULL;

    pool_allocator_func(p, allocator_type_free_all, 0, 0, NULL, 0, allocator_flag_none);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void* pool_allocator_func(void* allocator_data, allocator_type type, usz size, usz alignment, void* old_memory, usz old_size, allocator_flag flag) {

#pragma GCC diagnostic pop
    pool_allocator *p = (pool_allocator *)allocator_data;

    switch (type) {

    case allocator_type_alloc: {
            pool_free_node *node = p->head;

            if (node == NULL) {
                assert(0 && "Pool allocator has no free memory");
                return NULL;
            }

            p->head = p->head->next;

            if (flag & allocator_flag_clear_to_zero) {
                return memset(node, 0, p->chunk_size);
            }
            return node;
        }
    case allocator_type_realloc:
        assert(false && "Unsupported realloc on a pool allocator, a pool allocator only supports static sized items"); return NULL;
    case allocator_type_free: {
            pool_free_node *node;

            void *start = p->buf;
            void *end = &p->buf[p->buf_len];

            if (old_memory == NULL) {
                return NULL;
            }

            if (!(start <= old_memory && old_memory < end)) {
                assert(false && "Memory is out of bounds of the buffer in this pool");
                return NULL;
            }

            node = (pool_free_node *)old_memory;
            node->next = p->head;
            p->head = node;
            return NULL;
        }
    case allocator_type_free_all: {
            usz chunk_count = p->buf_len / p->chunk_size;
            usz i;

            for (i = 0; i < chunk_count; i++) {
                void *ptr = &p->buf[i * p->chunk_size];
                pool_free_node *node = (pool_free_node *)ptr;

                node->next = p->head;
                p->head = node;
            }
            return NULL;
        }
    }

    return NULL;
}

void* stack_allocator_func(void* allocator_data, allocator_type type, usz size, usz alignment, void* old_memory, usz old_size, allocator_flag flag) {
    stack_allocator *s = (stack_allocator *)allocator_data;
    assert(is_power_of_two(alignment));

    switch (type) {

    case allocator_type_alloc: {
            uptr curr_addr, next_addr;
            usz padding;
            stack_allocation_header *header;

            curr_addr = (uptr)s->buf + (uptr)s->curr_offset;
            padding = calc_padding_with_header(curr_addr, (uptr)alignment, sizeof(stack_allocation_header));
            if (s->curr_offset + padding + size > s->buf_len) {
                return NULL;
            }
            s->prev_offset = s->curr_offset;
            s->curr_offset += padding;

            next_addr = curr_addr + (uptr)padding;
            header = (stack_allocation_header *)(next_addr - sizeof(stack_allocation_header));
            header->padding = padding;
            header->prev_offset = s->prev_offset;

            s->curr_offset += size;

            if (flag & allocator_flag_clear_to_zero) {
                return memset((void *)next_addr, 0, size);
            }
            return (void *)next_addr;
        }
    case allocator_type_realloc: {
            if (old_memory == NULL) {
                return stack_allocator_func(s, allocator_type_alloc, size, alignment, old_memory, old_size, flag);
            } else if (size == 0) {
                return stack_allocator_func(s, allocator_type_free, size, alignment, old_memory, old_size, flag);
            } else {
                uptr start, end, curr_addr;
                usz min_size = old_size < size ? old_size : size;
                void *new_ptr;

                start = (uptr)s->buf;
                end = start + (uptr)s->buf_len;
                curr_addr = (uptr)old_memory;
                if (!(start <= curr_addr && curr_addr < end)) {
                    assert(false && "Out of bounds memory address passed to stack allocator for realloc");
                    return NULL;
                }

                if (curr_addr >= start + (uptr)s->curr_offset) {
                    return NULL;
                }

                if (old_size == size) {
                    return old_memory;
                }

                new_ptr = stack_allocator_func(s, allocator_type_alloc, size, alignment, old_memory, old_size, flag);
                memmove(new_ptr, old_memory, min_size);
                return new_ptr;
            }
        }
    case allocator_type_free: {
            if (old_memory != NULL) {
                uptr start, end, curr_addr;
                stack_allocation_header *header;
                usz prev_offset;

                start = (uptr)s->buf;
                end = start + (uptr)s->buf_len;
                curr_addr = (uptr)old_memory;

                if (!(start <= curr_addr && curr_addr < end)) {
                    assert(false && "Out of bounds memory address passed to stack allocator for free");
                    return NULL;
                }

                if (curr_addr >= start+(uptr)s->curr_offset) {
                    // Allow double frees
                    return NULL;
                }

                header = (stack_allocation_header *)(curr_addr - sizeof(stack_allocation_header));


                prev_offset = (usz)(curr_addr - (uptr)header->padding - start);

                if (prev_offset != header->prev_offset) {
                    assert(false && "Out of order stack allocator free");
                    return NULL;
                }

                s->curr_offset = s->prev_offset;
                s->prev_offset = header->prev_offset;
                return NULL;

            } else return NULL;
        }
    case allocator_type_free_all:
        s->curr_offset = 0;
        return NULL;
    }

    return NULL;
}


void stack_init(stack_allocator *s, void *backing_buffer, usz backing_buffer_length) {
    s->buf = (unsigned char *)backing_buffer;
    s->buf_len = backing_buffer_length;
    s->curr_offset = 0;
}


void* allocator_malloc(allocator *alloc, usz size, usz alignment, allocator_flag flag) {
    return alloc->func(alloc->allocator_data, allocator_type_alloc, size, alignment, NULL, 0, flag);
}

void* arena_allocator_func(void *allocator_data, allocator_type type, usz size, usz alignment, void *old_memory, usz old_size, allocator_flag flag) {
    arena_allocator *arena = (arena_allocator *)allocator_data;

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
