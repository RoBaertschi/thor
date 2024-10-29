#include "utils.hpp"
#include <cstdio>
#include <cstring>

int main(int argc, char **argv) {
    free_list_allocator fl = free_list_allocator{};
    unsigned char backing_buffer[256];
    free_list_init(&fl, backing_buffer, 256);
    allocator alloc = {};
    free_list_to_allocator(&fl, &alloc);

    char hello[] = "hello";
    zstr str = (zstr)allocator_malloc(&alloc, sizeof(hello));
    memcpy(str, hello, sizeof(hello));

    printf("%s\n", str);

    allocator_free_all(&alloc);
}
