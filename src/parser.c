#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include "ast.h"
#include "common.h"
#include "lexer.h"
#include "token.h"

ParseIndexResult module_insert_node(Module *m, Node node) {
    if (!vec_ensure_size(m->nodes.len, &m->nodes.cap, (void **)&m->nodes.data,
                         sizeof(Node), 1)) {
        return (ParseIndexResult){.type    = PARSE_RESULT_MALLOC_FAILED,
                                  .data.ok = 0};
    }

    m->nodes.data[m->nodes.len] = node;
    m->nodes.len += 1;

    return (ParseIndexResult){.type    = PARSE_RESULT_TYPE_OK,
                              .data.ok = m->nodes.len - 1};
}

void parser_next_token(Parser *p) {
    p->cur_token = p->peek_token;
    p->peek_token += 1;
}

Parser parser_create(Tokens t, str input) {
    Parser p = {
        .tokens     = t,
        .cur_token  = 1,
        .peek_token = 2,
        .input      = input,
        .cur_module = {.name = {0}, .nodes = {0}}
    };

    return p;
}

Token *parser_tok(Parser *p) {
    if (p->tokens.len <= p->cur_token) {
        return &p->tokens.tokens[p->tokens.len - 1];
    }

    return &p->tokens.tokens[p->cur_token];
}

Token *parser_peek_tok(Parser *p) {
    if (p->tokens.len <= p->peek_token) {
        return &p->tokens.tokens[p->tokens.len - 1];
    }

    return &p->tokens.tokens[p->peek_token];
}

ParseIndexResult parser_expect_peek(Parser *p, TokenType expected) {
    if (parser_peek_tok(p)->type != expected) {
        return (ParseIndexResult){
            .type                         = PARSE_RESULT_TYPE_UNEXPECTED_TOKEN,
            .data.errors.unexpected_token = {
                                             .expected = expected, .unexpected = parser_peek_tok(p)->type}
        };
    }
    parser_next_token(p);
    return (ParseIndexResult){.type    = PARSE_RESULT_TYPE_OK,
                              .data.ok = p->cur_token};
}

ParseNodeResult parse_integer(Parser *p) {
    Index main_token = p->cur_token;

    return (ParseNodeResult){
        .type    = PARSE_RESULT_TYPE_OK,
        .data.ok = {.data       = {0},
                    .type       = NODE_TYPE_INTEGER_LITERAL,
                    .main_token = main_token}
    };
}

ParseNodeResult parse_expression(Parser *p) {
    switch (parser_tok(p)->type) {
        case TOKEN_TYPE_INTEGER:
            return parse_integer(p);
        default:
            return (ParseNodeResult){
                .type                      = PARSE_RESULT_TYPE_NOT_EXPRESSION,
                .data.errors.invalid_token = {*parser_tok(p)}};
    }
}

ParseNodeResult parse_variable_declaration(Parser *p) {
    Index main_token = p->cur_token;
    Index lhs        = 0;
    Index rhs        = 0;

    TRY(parser_expect_peek(p, TOKEN_TYPE_COLON), ParseIndexResult,
        ParseNodeResult);
    if (parser_peek_tok(p)->type == TOKEN_TYPE_IDENTIFIER) {
        parser_next_token(p);
        lhs = p->cur_token;
    }
    TRY(parser_expect_peek(p, TOKEN_TYPE_EQUAL), ParseIndexResult,
        ParseNodeResult);
    parser_next_token(p);
    ParseNodeResult expr = parse_expression(p);
    if (expr.type != PARSE_RESULT_TYPE_OK) {
        return expr;
    }
    ParseIndexResult idx = module_insert_node(&p->cur_module, expr.data.ok);
    if (idx.type != PARSE_RESULT_TYPE_OK) {
        return (ParseNodeResult){.type        = idx.type,
                                 .data.errors = idx.data.errors};
    }
    rhs = idx.data.ok;
    TRY(parser_expect_peek(p, TOKEN_TYPE_EOL), ParseIndexResult,
        ParseNodeResult);
    return (ParseNodeResult){
        .type    = PARSE_RESULT_TYPE_OK,
        .data.ok = (Node){.data       = {.rhs = rhs, .lhs = lhs},
                          .type       = NODE_TYPE_VARIABLE_DECLARATION,
                          .main_token = main_token}
    };
}

// Protocol: after parse_node p->cur_token is on the token after the previous
// node
ParseNodeResult parse_node(Parser *p) {
    switch (parser_tok(p)->type) {

        case TOKEN_TYPE_IDENTIFIER:
            return parse_variable_declaration(p);
        case TOKEN_TYPE_INTEGER:
        case TOKEN_TYPE_COLON:
        case TOKEN_TYPE_EQUAL:
        case TOKEN_TYPE_EOL:
        case TOKEN_TYPE_EOF:
        case TOKEN_TYPE_NONE:
        case TOKEN_TYPE_INVALID:
        case TOKEN_TYPE_LAST:
            return (ParseNodeResult){
                .type                      = PARSE_RESULT_TYPE_INVALID,
                .data.errors.invalid_token = {*parser_tok(p)},
            };
    }
    return (ParseNodeResult){
        .type                      = PARSE_RESULT_TYPE_INVALID,
        .data.errors.invalid_token = {*parser_tok(p)},
    };
}

ParseModuleResult parser_parse_module(Parser *p) {
    p->cur_module = (Module){.name = to_str("main"), .nodes = {0}};

    while (parser_tok(p)->type != TOKEN_TYPE_EOF) {
        ParseNodeResult result = parse_node(p);
        if (result.type != PARSE_RESULT_TYPE_OK) {
            return (ParseModuleResult){.type        = result.type,
                                       .data.errors = result.data.errors};
        }
    }

    return (ParseModuleResult){.type    = PARSE_RESULT_TYPE_OK,
                               .data.ok = p->cur_module};
}

void module_destroy(Module m) {
    free(m.nodes.data);
    str_destroy(m.name);
}

void parser_destroy(Parser p) {
    str_destroy(p.input);
    tokens_destroy(p.tokens);
}


void print_module(Module *m) {
    char *name = to_cstr(m->name);
    printf("Module %s:\n", name);
    free(name);
    for (usz i = 0; i < m->nodes.len; i++) {
        Node *node = &m->nodes.data[i];
        switch (node->type) {

            case NODE_TYPE_VARIABLE_DECLARATION:
                printf("variable declaration");
                break;
            case NODE_TYPE_INTEGER_LITERAL:
                printf("integer literal");
                break;
        }
    }
}
