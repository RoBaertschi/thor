#pragma once

#include "utils.hpp"

// X macro list
#define TOKEN_LIST      \
    X(NEWLINE, newline) \
    X(COLON, colon)     \
    X(EQUAL, equal)     \
    X(NUMBER, number)

enum TokenType : u8 {
#define X(name, unused) TOKEN_##name,
    TOKEN_LIST
#undef X
};

inline izstr token_type_to_string(TokenType type) {
    switch (type) {
#define X(name, string) case TOKEN_##name: return #string;
    TOKEN_LIST
#undef X
    }
}

struct Token {
    u32 pos;
    u32 length;
};
