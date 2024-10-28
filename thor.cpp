#include "utils.hpp"
#include <cstdio>
#include <cstring>

int main(int argc, char **argv) {
    arena_allocator a = arena_allocator{0};
    unsigned char backing_buffer[256];
    arena_init(&a, backing_buffer, 256);
    allocator alloc = {0};
    arena_to_allocator(&a, &alloc);

    char hello[] = "hello";
    zstr str = (zstr)allocator_malloc(&alloc, sizeof(hello));
    memcpy(str, hello, sizeof(hello));

    printf("%s\n", str);
}
