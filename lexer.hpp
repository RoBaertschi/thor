#pragma once

#include "utils.hpp"

// X macro list
#define TOKEN_LIST              \
    X(IDENTIFIER, identifier)   \
    X(NEWLINE, newline)         \
    X(COLON, colon)             \
    X(EQUAL, equal)             \
    X(NUMBER, number)

enum token_type : u8 {
#define X(name, unused) TOKEN_##name,
    TOKEN_LIST
#undef X
};

inline izstr token_type_to_string(token_type type) {
    switch (type) {
#define X(name, string) case TOKEN_##name: return #string;
    TOKEN_LIST
#undef X
    }
}

typedef struct token token;
struct token {
    u32         pos;
    u32         length;
    token_type   type;
};

typedef struct lexer lexer;
struct lexer {
    allocator a;
};
