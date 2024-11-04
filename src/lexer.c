#include "lexer.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "token.h"

void NORETURN lexer_fatal_error(Lexer *l, LexerFatalError error) {
    if (l->fatal_error_cb != NULL) {
        l->fatal_error_cb(error);
    }
    fprintf(stderr, "Could not allocate memory, aborting...");
    abort();
}

void lexer_read_char(Lexer *l) {
    if (l->input.len <= l->peek_pos) {
        l->ch = 0; // Set ch to EOF
    } else {
        l->ch = l->input.ptr[l->peek_pos];
    }
    l->pos = l->peek_pos;
    l->peek_pos += 1;
}

Lexer lexer_create(str input, LexerFatalErrorCallback fatal_error_cb) {
    Lexer lexer = {.input          = str_clone(input),
                   .ch             = 0,
                   .pos            = 0,
                   .peek_pos       = 0,
                   .fatal_error_cb = fatal_error_cb};

    lexer_read_char(&lexer);

    return lexer;
}

void lexer_destroy(Lexer lexer) { str_destroy(lexer.input); }

void tokens_destroy(Tokens t) {
    free(t.extra_data.data);
    free(t.tokens);
}

Index tokens_insert(Lexer *l, Tokens *t, Token token) {
    if (!vec_ensure_size(t->len, &t->cap, (void *)&t->tokens, sizeof(Token),
                         1)) {
        lexer_fatal_error(l, LEXER_FATAL_ERROR_MALLOC_FAILED);
    }

    Index idx         = t->len;
    t->tokens[t->len] = token;
    t->len += 1;
    return idx;
}

TokenExtraDataIndex extra_data_insert(Lexer *l, Tokens *t, TokenExtraData ed) {
    if (!vec_ensure_size(t->extra_data.len, &t->extra_data.cap,
                         (void *)&t->extra_data.data, sizeof(TokenExtraData),
                         1)) {
        lexer_fatal_error(l, LEXER_FATAL_ERROR_MALLOC_FAILED);
    }
    TokenExtraDataIndex idx               = t->extra_data.len;
    t->extra_data.data[t->extra_data.len] = ed;
    t->extra_data.len += 1;
    return idx;
}

Index tokens_insert_extra(Lexer *l, Tokens *t, Token *token,
                          TokenExtraData ed) {
    TokenExtraDataIndex ed_idx = extra_data_insert(l, t, ed);
    token->extra_data          = ed_idx;
    return tokens_insert(l, t, *token);
}

bool is_ident_char(char ch) {
    return ('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z');
}

bool is_number(char ch) { return '0' <= ch && ch <= '9'; }

Token lexer_read_identifier(Lexer *l, Tokens *t) {
    usz pos = l->pos;
    lexer_read_char(l);

    while (is_ident_char(l->ch) || is_number(l->ch)) {
        lexer_read_char(l);
    }

    Token token = {.pos        = pos,
                   .len        = l->pos - pos,
                   .type       = TOKEN_TYPE_IDENTIFIER,
                   .extra_data = 0};

    tokens_insert(l, t, token);
    return token;
}

Token lexer_read_number(Lexer *l, Tokens *t) {
    usz pos    = l->pos;
    int number = 0;

    while (is_number(l->ch)) {
        number *= 10;
        number += l->ch - '0';
        /*printf("ch: '%c' number: %d\n", l->ch, number);*/
        lexer_read_char(l);
    }

    TokenExtraData data = {.type = EXTRA_DATA_INTEGER,
                           .data = {.integer = number}};
    Token token = {.type = TOKEN_TYPE_INTEGER, .pos = pos, .len = l->pos - pos};
    tokens_insert_extra(l, t, &token, data);
    return token;
}

void lexer_skip_whitespace(Lexer *l) {
    while (l->ch == '\r' || l->ch == ' ' || l->ch == '\t') {
        lexer_read_char(l);
    }
}

Token lexer_next_token(Lexer *l, Tokens *t) {
    lexer_skip_whitespace(l);

    Token cur_token = {0};
    cur_token.pos   = l->pos;

    switch (l->ch) {
        case ':':
            cur_token.type = TOKEN_TYPE_COLON;
            cur_token.len  = 1;
            break;
        case '=':
            cur_token.type = TOKEN_TYPE_EQUAL;
            cur_token.len  = 1;
            break;
        case '\n':
            cur_token.type = TOKEN_TYPE_EOL;
            cur_token.len  = 1;
            break;
        case 0:
            cur_token.type = TOKEN_TYPE_EOF;
            cur_token.len  = 1;
            break;

        default: {
            if (is_ident_char(l->ch)) {
                return lexer_read_identifier(l, t);
            }
            if (is_number(l->ch)) {
                return lexer_read_number(l, t);
            }

            cur_token.type = TOKEN_TYPE_INVALID;
            cur_token.len  = 1;
        }
    }

    tokens_insert(l, t, cur_token);
    lexer_read_char(l);
    return cur_token;
}

// This inserts the NONE Token, used in the parser.
void tokens_init(Lexer *l, Tokens *t) {
    tokens_insert(
        l, t,
        (Token){.type = TOKEN_TYPE_NONE, .len = 0, .pos = 0, .extra_data = 0});
}

Tokens lexer_lex_tokens(Lexer *l) {
    Tokens tokens = {0};
    tokens_init(l, &tokens);
    Token token = lexer_next_token(l, &tokens);

    while (token.type != TOKEN_TYPE_EOF) {
        token = lexer_next_token(l, &tokens);
    }

    return tokens;
}

str tokens_token_str(str input, Tokens *t, Index idx) {
    return to_strl(input.ptr + t->tokens[idx].pos, t->tokens[idx].len);
}

int extra_data_integer(Tokens *t, TokenExtraDataIndex i) {
    assert(t->extra_data.len > i);
    assert(t->extra_data.data[i].type == EXTRA_DATA_INTEGER);

    return t->extra_data.data[i].data.integer;
}

void print_tokens(Tokens *t) {
    for (usz i = 0; i < t->len; i++) {
        printf("%s\n", token_type_str(t->tokens[i].type));
    }
}
