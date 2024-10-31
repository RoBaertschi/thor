#include "utils.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>

int main(int argc, char **argv) {
    free_list_allocator fl = free_list_allocator{};
    void *backing_buffer = malloc(256);
    free_list_init(&fl, backing_buffer, 256);
    allocator alloc = {};
    free_list_to_allocator(&fl, &alloc);

    char hello[] = "hello";
    zstr str = (zstr)allocator_malloc(alloc, sizeof(hello));
    memcpy(str, hello, sizeof(hello));
    struct str str2 = str_literal(alloc, "");


    printf("str\t: '%s'\nstr2\t: '%s'\n", str, str_to_zstr(&str2));
    str_free(str2);
    memcpy(str, hello, sizeof(hello));

    allocator_free(alloc, allocator_malloc(alloc, 8));
    allocator_free(alloc, allocator_malloc(alloc, 8));
    allocator_malloc(alloc, 8);
    allocator_malloc(alloc, 8);

    struct str str3 = str_literal(alloc, "");
    printf("str3\t: '%s'\n", str_to_zstr(&str3));

    allocator_free_all(alloc);
    free(backing_buffer);
}
