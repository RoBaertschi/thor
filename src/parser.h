#pragma once

#include "ast.h"
#include "lexer.h"
#include "token.h"

enum ParseResultType {
    PARSE_RESULT_TYPE_OK,
    // No data
    PARSE_RESULT_MALLOC_FAILED,
    // errors.unexpected_token
    PARSE_RESULT_TYPE_UNEXPECTED_TOKEN,
    // Found an invalid token.
    PARSE_RESULT_TYPE_INVALID,
    // Could not parse the expression. The invalid token is populatet with the
    // problematic token.
    PARSE_RESULT_TYPE_NOT_EXPRESSION,
    // Expected a RPAREN or an identifier, instead found a invalid token. The
    // invalid token ist saved in invalid token.
    PARSE_RESULT_TYPE_EXPECTED_FUNCTION_ARGUMENT_LIST,
};
typedef enum ParseResultType        ParseResultType;

typedef struct UnexpectedTokenError UnexpectedTokenError;
struct UnexpectedTokenError {
    TokenType expected;
    TokenType unexpected;
};

typedef struct InvalidTokenError InvalidTokenError;
struct InvalidTokenError {
    Token token;
};

typedef union ParseErrors ParseErrors;
union ParseErrors {
    UnexpectedTokenError unexpected_token;
    InvalidTokenError    invalid_token;
};

#define PARSER_RESULT(ok_type)                                    \
    typedef struct Parse##ok_type##Result Parse##ok_type##Result; \
    struct Parse##ok_type##Result {                               \
        ParseResultType type;                                     \
        union Parse##ok_type##ResultUnion {                       \
            ok_type     ok;                                       \
            ParseErrors errors;                                   \
        } data;                                                   \
    };

PARSER_RESULT(Module)
PARSER_RESULT(Node)
PARSER_RESULT(Index)
PARSER_RESULT(FunctionArguments)

str parse_error_str(ParseResultType type, ParseErrors errors);

/*#define TRY(result, type) (result.type == PARSE_RESULT_TYPE_OK ?
 * result.data.ok : (type) {.type = result.type, .data.errors =
 * result.data.errors})*/
#define TRY(result, from, to)                                                  \
    do {                                                                       \
        from tried = (result);                                                 \
        if (tried.type != PARSE_RESULT_TYPE_OK) {                              \
            return (to){.type = tried.type, .data.errors = tried.data.errors}; \
        }                                                                      \
    } while (0);

#define TRY_OUTPUT(result, from, to, output_name)                         \
    do {                                                                  \
        Parse##from##Result tried = (result);                             \
        if (tried.type != PARSE_RESULT_TYPE_OK) {                         \
            return (Parse##to##Result){.type        = tried.type,         \
                                       .data.errors = tried.data.errors}; \
        }                                                                 \
        output_name = tried.data.ok;                                      \
    } while (0);

typedef struct Parser Parser;
struct Parser {
    str    input;
    Tokens tokens;
    Index  cur_token;
    Index  peek_token;
    Module cur_module;
};

Parser            parser_create(Tokens t, str input);
ParseModuleResult parser_parse_module(Parser *p);
void              parser_destroy(Parser p);

void              module_destroy(Module m);
void              print_module(Parser *p, Module *m);
