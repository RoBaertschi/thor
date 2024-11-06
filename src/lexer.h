#pragma once

#include "common.h"
#include "token.h"
#include "uthash.h"

enum TokenExtraDataType {
    EXTRA_DATA_INTEGER,
};
typedef enum TokenExtraDataType TokenExtraDataType;

typedef struct TokenExtraData TokenExtraData;
struct TokenExtraData {
    TokenExtraDataType type;
    union {
        int integer;
    } data;
};

typedef usz           Index;
typedef struct Tokens Tokens;
struct Tokens {
    usz    len;
    usz    cap;
    Token *tokens;

    struct {
        usz             len;
        usz             cap;
        TokenExtraData *data;
    } extra_data;
};

void print_tokens(Tokens *t);

enum LexerFatalError {
    LEXER_FATAL_ERROR_MALLOC_FAILED,
};
typedef enum LexerFatalError LexerFatalError;

typedef struct keyword_to_token keyword_to_token;
struct keyword_to_token {
    char const    *keyword;
    TokenType      token;
    UT_hash_handle hh;
};

// You should not return from this function, if you do, we just abort ourselves
// with an error message.
typedef void (*LexerFatalErrorCallback)(LexerFatalError error);

typedef struct Lexer Lexer;
struct Lexer {
    char                    ch;
    usz                     pos;
    usz                     peek_pos;
    str                     input;
    LexerFatalErrorCallback fatal_error_cb;
};


Lexer  lexer_create(str input, LexerFatalErrorCallback fatal_error_cb);
void   lexer_destroy(Lexer lexer);
Tokens lexer_lex_tokens(Lexer *l);

keyword_to_token *get_keyword_to_tokens_hash_map(void);
void free_global_resources(void);

str    tokens_token_str(str input, Tokens *t, Index idx);
void   tokens_destroy(Tokens t);

int extra_data_integer(Tokens *t, TokenExtraDataIndex i);
