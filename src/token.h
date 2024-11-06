#pragma once

#include "common.h"

// Token X macro list
#define TOKENS                                                         \
    /* Special   */                                                    \
    X(NONE, none) /* This is a special Token, used to indicate that an \
                     optional Token is missing */                      \
    X(INVALID, invalid)                                                \
    X(EOF, eof)                                                        \
    /* Literals  */                                                    \
    X(IDENTIFIER, identifier)                                          \
    X(INTEGER, integer)                                                \
    /* Delimiters */                                                   \
    X(COLON, colon)                                                    \
    X(EQUAL, equal)                                                    \
    /* Barces, Brackets... */                                          \
    X(LPAREN, lparen)                                                  \
    X(RPAREN, rparen)                                                  \
    X(LBRACE, lbrace)                                                  \
    X(RBRACE, rbrace)                                                  \
    /* Keywords */                                                     \
    X(FN, fn)                                                          \
    /* Whitespace */                                                   \
    X(EOL, eol)

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
