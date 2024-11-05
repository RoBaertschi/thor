#pragma once

#include "common.h"

// Token X macro list
#define TOKENS                                                         \
    X(NONE, none) /* This is a special Token, used to indicate that an \
                     optional Token is missing */                      \
    X(INVALID, invalid)                                                \
    X(IDENTIFIER, identifier)                                          \
    X(INTEGER, integer)                                                \
    X(COLON, colon)                                                    \
    X(EQUAL, equal)                                                    \
    X(EOL, eol)                                                        \
    X(EOF, eof)

enum TokenType {
#define X(name, unused) TOKEN_TYPE_##name,
    TOKENS
#undef X
        TOKEN_TYPE_LAST,
};
typedef enum TokenType TokenType;
typedef usz            TokenExtraDataIndex;

char const *token_type_str(TokenType type);

typedef struct Token Token;
struct Token {
    TokenType           type;
    usz                 pos;
    usz                 len;
    TokenExtraDataIndex extra_data;
};
