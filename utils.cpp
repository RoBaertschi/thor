#include "utils.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>


void* allocator_malloc(allocator alloc, usz size, usz alignment, allocator_flag flag) {
    return alloc.func(alloc.allocator_data, allocator_type_alloc, size, alignment, NULL, 0, flag);
}

void* allocator_realloc(allocator alloc, void * old_memory, usz old_size, usz size, usz alignment, allocator_flag flag) {
    return alloc.func(alloc.allocator_data, allocator_type_realloc, size, alignment, old_memory, old_size, flag);
}
void allocator_free_all(allocator alloc) {
    (void)alloc.func(alloc.allocator_data, allocator_type_free_all, 0, 0, NULL, 0, allocator_flag_none);
}
void allocator_free(allocator alloc, void *to_free) {
    (void)alloc.func(alloc.allocator_data, allocator_type_free, 0, 0, to_free, 0, allocator_flag_none);
}

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

void free_list_node_insert(free_list_node **phead, free_list_node *prev_node, free_list_node *new_node) {
    if (prev_node == NULL) {
        if (*phead != NULL) {
            new_node->next = *phead;
        } else {
            *phead = new_node;
        }
    } else {
        if (prev_node->next == NULL) {
            prev_node->next = new_node;
            new_node->next = NULL;
        } else {
            new_node->next = prev_node->next;
            prev_node->next = new_node;
        }
    }
}

void free_list_node_remove(free_list_node **phead, free_list_node *prev_node, free_list_node *del_node) {
    if (prev_node == NULL) {
        *phead = del_node->next;
    } else {
        prev_node->next = del_node->next;
    }
}

void free_list_coalescence(free_list_allocator *fl, free_list_node *prev_node, free_list_node *free_node) {
    if (free_node->next != NULL && (void *)((char *)free_node + free_node->block_size) == free_node->next) {
        free_node->block_size += free_node->next->block_size;
        free_list_node_remove(&fl->head, free_node, free_node->next);
    }
    if (prev_node->next != NULL && (void *)((char *)prev_node + prev_node->block_size) == free_node) {
        prev_node->block_size += free_node->next->block_size;
        free_list_node_remove(&fl->head, prev_node, free_node);
    }
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
    return best_node;
}

void *free_list_allocator_func (void* allocator_data, allocator_type type, usz size, usz alignment, void* old_memory, usz old_size, allocator_flag flag) {
    free_list_allocator *fl = (free_list_allocator *)allocator_data;

    switch (type) {

    case allocator_type_alloc: {
            usz padding = 0;
            free_list_node *prev_node = NULL;
            free_list_node *node = NULL;
            usz alignment_padding, required_space, remaining;
            free_list_allocation_header *header_ptr;

            if (size < sizeof(free_list_node)) {
                size = sizeof(free_list_node);
            }
            if (alignment < 8) {
                alignment = 8;
            }

            if (fl->policy == placement_policy_find_best) {
                node = free_list_find_best(fl, size, alignment, &padding, &prev_node);
            } else {
                node = free_list_find_first(fl, size, alignment, &padding, &prev_node);
            }

            if (node == NULL) {
                assert(0 && "Free list has no memory");
                return NULL;
            }

            alignment_padding = padding - sizeof(free_list_allocation_header);
            required_space = size + padding;
            remaining = node->block_size - required_space;

            if (remaining > 0) {
                free_list_node *new_node = (free_list_node *)((char *)node + required_space);
                new_node->block_size = remaining;
                free_list_node_insert(&fl->head, node, new_node);
            }

            free_list_node_remove(&fl->head, prev_node, node);

            header_ptr = (free_list_allocation_header *)((char *)node + alignment_padding);
            header_ptr->block_size = required_space;
            header_ptr->padding = alignment_padding;

            fl->used += required_space;

            return (void *)((char *)header_ptr + sizeof(free_list_allocation_header));
        }
    case allocator_type_realloc: {
            void * new_space = free_list_allocator_func(fl, allocator_type_alloc, size, alignment, NULL, 0, flag);
            new_space = memmove(new_space, old_memory, old_size);
            (void)free_list_allocator_func(fl, allocator_type_free, 0, 0, old_memory, 0, flag);
            return new_space;
        }
    case allocator_type_free: {
            free_list_allocation_header *header;
            free_list_node *free_node;
            free_list_node *node;
            free_list_node *prev_node = NULL;

            if (old_memory == NULL) {
                return NULL;
            }

            header = (free_list_allocation_header *)((char *)old_memory - sizeof(free_list_allocation_header));
            free_node = (free_list_node *)header;
            free_node->block_size = header->block_size + header->padding;
            free_node->next = NULL;

            node = fl->head;
            while (node != NULL) {
                if (old_memory < node) {
                    free_list_node_insert(&fl->head, prev_node, free_node);
                    break;
                }
                prev_node = node;
                node = node->next;
            }

            fl->used -= free_node->block_size;

            free_list_coalescence(fl, prev_node, free_node);

            return NULL;
        }
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
    free_list_allocator_func(fl, allocator_type_free_all, 0, 0, NULL, 0, allocator_flag_none);
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

str str_literal(allocator a, izstr str) {

    usz length = strlen(str);
    if (length == 0) {
        return { /*.len =*/ 0, /*.ptr =*/ NULL, /*.alloc =*/ a };
    }

    char *new_string = (char *) allocator_malloc(a, length);
    memcpy(new_string, str, length);

    return { /*.len =*/ length, /*.ptr =*/ new_string, /*.alloc =*/ a };
}

zstr str_to_zstr(str *str) {
    zstr new_string = (zstr) allocator_malloc(str->alloc, str->len + 1 /* \0 */);
    if (str->len > 0) {
        memcpy(new_string, str->ptr, str->len);
    }
    new_string[str->len] = '\0';
    return new_string;
}
zstr str_to_zstr(str *str, allocator a) {
    zstr new_string = (zstr) allocator_malloc(a, str->len + 1 /* \0 */);
    if (str->len > 0) {
        memcpy(new_string, str->ptr, str->len);
    }
    new_string[str->len] = '\0';
    return new_string;
}

void str_free(str str) {
    allocator_free(str.alloc, str.ptr);
}

void str_buffer_append(str_buffer *buf, str str) {
    usz new_size = buf->len + str.len;
    while (new_size > buf->cap) {
        buf->buf = (char *)allocator_realloc(buf->a, buf->buf, buf->cap, buf->cap * 2);
        buf->cap = buf->cap * 2;
    }
    memcpy(buf->buf + buf->len, str.ptr, str.len);
    buf->len += str.len;
}

void str_buffer_append(str_buffer *buf, izstr str) {
    usz strsize = strlen(str);
    usz new_size = buf->len + strsize;
    while (new_size > buf->cap) {
        buf->buf = (char *)allocator_realloc(buf->a, buf->buf, buf->cap, buf->cap * 2);
        buf->cap = buf->cap * 2;
    }
    memcpy(buf->buf + buf->len, str, strsize);
    buf->len += strsize;
}
