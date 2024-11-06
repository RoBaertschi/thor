#include "parser.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "ast.h"
#include "common.h"
#include "da.h"
#include "lexer.h"
#include "token.h"

ParseIndexResult module_insert_node(Module *m, Node node) {
    da_append(&m->nodes, node);

    return (ParseIndexResult){.type    = PARSE_RESULT_TYPE_OK,
                              .data.ok = m->nodes.count - 1};
}

ParseIndexResult module_insert_top_level_node(Module *m, Index node) {
    da_append(&m->top_level_nodes, node);

    return (ParseIndexResult){.type    = PARSE_RESULT_TYPE_OK,
                              .data.ok = m->top_level_nodes.count - 1};
}

ParseIndexResult module_insert_extra_data(Module *m, NodeExtraData ed) {
    da_append(&m->extra_data, ed);

    return (ParseIndexResult){.type    = PARSE_RESULT_TYPE_OK,
                              .data.ok = m->extra_data.count - 1};
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
        .cur_module = {.name = to_str("main"), .nodes = {0}}
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

ParseIndexResult parser_expect(Parser *p, TokenType expected) {
    if (parser_tok(p)->type != expected) {
        return (ParseIndexResult){
            .type                         = PARSE_RESULT_TYPE_UNEXPECTED_TOKEN,
            .data.errors.unexpected_token = {
                                             .expected = expected, .unexpected = parser_tok(p)->type}
        };
    }
    return (ParseIndexResult){.type    = PARSE_RESULT_TYPE_OK,
                              .data.ok = p->cur_token};
}

ParseNodeResult parse_node(Parser *p);

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
    parser_next_token(p);
    return (ParseNodeResult){
        .type    = PARSE_RESULT_TYPE_OK,
        .data.ok = (Node){.data       = {.rhs = rhs, .lhs = lhs},
                          .type       = NODE_TYPE_VARIABLE_DECLARATION,
                          .main_token = main_token}
    };
}

ParseNodeResult parse_block(Parser *p) {
    // { ... <-
    parser_next_token(p);
    Index     main_token = p->cur_token;
    BlockData block_data = {0};

    while (parser_tok(p)->type != TOKEN_TYPE_RBRACE) {
        if (parser_tok(p)->type == TOKEN_TYPE_EOL) {
            parser_next_token(p);
            continue;
        }
        Index idx;
        Node  out_node;

        TRY_OUTPUT(parse_node(p), Node, Node, out_node);
        TRY_OUTPUT(module_insert_node(&p->cur_module, out_node), Index, Node,
                   idx);

        da_append(&block_data, idx);
    }

    parser_next_token(p);

    Index         ed_idx;
    NodeExtraData ed = {.type       = NODE_EXTRA_DATA_BLOCK,
                        .data.block = block_data};
    TRY_OUTPUT(module_insert_extra_data(&p->cur_module, ed), Index, Node,
               ed_idx);

    return (ParseNodeResult){
        .type    = PARSE_RESULT_TYPE_OK,
        .data.ok = {.type       = NODE_TYPE_BLOCK,
                    .main_token = main_token,
                    .data.lhs   = ed_idx}
    };
}

// Expects cur_token to be on an identifier.
ParseFunctionArgumentsResult parse_arguments(Parser *p) {
    FunctionArguments args = {0};
    do {
        Index name_index, type_index;
        name_index = p->cur_token;
        TRY(parser_expect_peek(p, TOKEN_TYPE_IDENTIFIER), ParseIndexResult,
            ParseFunctionArgumentsResult);
        type_index = p->cur_token;
        if (parser_peek_tok(p)->type != TOKEN_TYPE_COMMA) {
            break;
        }
        parser_next_token(p);
        FunctionArgument arg = {.type = type_index, .name = name_index};
        da_append(&args, arg);
    } while (parser_tok(p)->type == TOKEN_TYPE_IDENTIFIER);

    return (ParseFunctionArgumentsResult){.type    = PARSE_RESULT_TYPE_OK,
                                          .data.ok = args};
}

ParseNodeResult parse_function_defintition(Parser *p) {
    Index             main_token, return_type;
    FunctionArguments args;

    // fn name_of_function <-
    TRY_OUTPUT(parser_expect_peek(p, TOKEN_TYPE_IDENTIFIER), Index, Node,
               main_token);
    // fn name_of_function( <-
    TRY(parser_expect_peek(p, TOKEN_TYPE_LPAREN), ParseIndexResult,
        ParseNodeResult);

    switch (parser_peek_tok(p)->type) {
        case TOKEN_TYPE_IDENTIFIER:

            parser_next_token(p);
            TRY_OUTPUT(parse_arguments(p), FunctionArguments, Node, args);

            // ) <-
            TRY(parser_expect_peek(p, TOKEN_TYPE_RPAREN), ParseIndexResult,
                ParseNodeResult);

            break;
        case TOKEN_TYPE_RPAREN:
            // ) <-
            TRY(parser_expect_peek(p, TOKEN_TYPE_RPAREN), ParseIndexResult,
                ParseNodeResult);
            break;
        default:
            return (ParseNodeResult){
                .type = PARSE_RESULT_TYPE_EXPECTED_FUNCTION_ARGUMENT_LIST,
                .data.errors.invalid_token =
                    {
                                                .token = *parser_peek_tok(p),
                                                },
            };
    }

    // fn name(arg type) <-

    // fn name(arg type) returntype <-
    // TODO: Type parsing
    TRY_OUTPUT(parser_expect_peek(p, TOKEN_TYPE_IDENTIFIER), Index, Node,
               return_type);

    parser_expect_peek(p, TOKEN_TYPE_LBRACE);
    // fn name(arg type) returntype { <-
    Node  block;
    Index block_idx;
    TRY_OUTPUT(parse_block(p), Node, Node, block);
    TRY_OUTPUT(module_insert_node(&p->cur_module, block), Index, Node,
               block_idx);

    NodeExtraData extra_data = {
        .type                    = NODE_EXTRA_DATA_FUNCTION_PROTOTYPE,
        .data.function_prototype = {.return_type = return_type, .args = args}
    };
    da_append(&p->cur_module.extra_data, extra_data);
    Index ed_idx = p->cur_module.extra_data.count - 1;

    Node node = {
        .data       = {.lhs = ed_idx, .rhs = block_idx},
        .type       = NODE_TYPE_FUNCTION_DEFINITION,
        .main_token = main_token,
    };

    return (ParseNodeResult){.type = PARSE_RESULT_TYPE_OK, .data.ok = node};
}

// Protocol: after parse_node p->cur_token is on the token after the previous
// node
ParseNodeResult parse_node(Parser *p) {
    switch (parser_tok(p)->type) {
        case TOKEN_TYPE_FN:
            return parse_function_defintition(p);
        case TOKEN_TYPE_IDENTIFIER:
            return parse_variable_declaration(p);
        default:
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

        Index idx;
        TRY_OUTPUT(module_insert_node(&p->cur_module, result.data.ok), Index,
                   Module, idx);

        TRY(module_insert_top_level_node(&p->cur_module, idx), ParseIndexResult,
            ParseModuleResult);
    }

    return (ParseModuleResult){.type    = PARSE_RESULT_TYPE_OK,
                               .data.ok = p->cur_module};
}

void module_destroy(Module m) {
    free(m.nodes.items);
    free(m.top_level_nodes.items);
    str_destroy(m.name);
}

void parser_destroy(Parser p) {
    str_destroy(p.input);
    tokens_destroy(p.tokens);
}

void print_node(Parser *p, Module *m, Node *node);

void print_integer(Parser *p, Module *m, Node *node) {
    (void)m;
    str integer = tokens_token_str(p->input, &p->tokens, node->main_token);
    str_fprint(stdout, integer);
    str_destroy(integer);
}

void print_variable_declaration(Parser *p, Module *m, Node *node) {
    str var_name = tokens_token_str(p->input, &p->tokens, node->main_token);
    str_fprint(stdout, var_name);
    str_destroy(var_name);
    printf(" :");
    if (node->data.lhs != 0) {
        printf(" ");
        str type_name = tokens_token_str(p->input, &p->tokens, node->data.lhs);
        str_fprint(stdout, type_name);
        str_destroy(type_name);
        printf(" ");
    }
    printf("= ");
    print_node(p, m, &m->nodes.items[node->data.rhs]);
    printf("\n");
}

void print_block(Parser *p, Module *m, Node *node) {
    printf("{\n");
    BlockData bd = m->extra_data.items[node->data.lhs].data.block;
    for (usz i = 0; i < bd.count; i++) {
        printf("\t");
        print_node(p, m, &m->nodes.items[bd.items[i]]);
    }
    printf("\n}\n");
}

void print_function_definition(Parser *p, Module *m, Node *node) {
    FunctionPrototypeData fpd =
        m->extra_data.items[node->data.lhs].data.function_prototype;

    str name = tokens_token_str(p->input, &p->tokens, node->main_token);
    printf("fn ");
    str_fprint(stdout, name);
    str_destroy(name);
    printf("(");

    for (usz i = 0; i < fpd.args.count; i++) {
        str var_name, type;
        var_name =
            tokens_token_str(p->input, &p->tokens, fpd.args.items[i].name);
        type = tokens_token_str(p->input, &p->tokens, fpd.args.items[i].type);

        str_fprint(stdout, var_name);
        printf(" ");
        str_fprint(stdout, type);
        if (i + 1 < fpd.args.count) {
            printf(", ");
        }
        str_destroy(var_name);
        str_destroy(type);
    }
    printf(") ");
    str return_type = tokens_token_str(p->input, &p->tokens, fpd.return_type);
    str_fprint(stdout, return_type);
    str_destroy(return_type);
    printf(" ");

    print_block(p, m, &m->nodes.items[node->data.rhs]);
}

void print_node(Parser *p, Module *m, Node *node) {

    switch (node->type) {

        case NODE_TYPE_VARIABLE_DECLARATION:
            print_variable_declaration(p, m, node);
            break;
        case NODE_TYPE_INTEGER_LITERAL:
            print_integer(p, m, node);
            break;

        case NODE_TYPE_BLOCK:
            print_block(p, m, node);
            break;
        case NODE_TYPE_FUNCTION_DEFINITION:
            print_function_definition(p, m, node);
            break;
    }
}

void print_module(Parser *p, Module *m) {
    char *name = to_cstr(m->name);
    printf("Module %s:\n", name);
    free(name);
    for (usz i = 0; i < m->top_level_nodes.count; i++) {
        Node *node = &m->nodes.items[m->top_level_nodes.items[i]];
        print_node(p, m, node);
    }
}

str parse_error_str(ParseResultType type, ParseErrors errors) {
    switch (type) {

        case PARSE_RESULT_TYPE_OK:
            return to_str("ok");
        case PARSE_RESULT_MALLOC_FAILED:
            return to_str("malloc failed");
        case PARSE_RESULT_TYPE_UNEXPECTED_TOKEN:

            return str_format(
                "unexpected token, expected: %s, got: %s",
                token_type_str(errors.unexpected_token.expected),
                token_type_str(errors.unexpected_token.unexpected));
        case PARSE_RESULT_TYPE_INVALID:
            return str_format("invalid token %s at pos %zu",
                              token_type_str(errors.invalid_token.token.type),
                              errors.invalid_token.token.pos);
        case PARSE_RESULT_TYPE_NOT_EXPRESSION:
            return str_format("token %s not an expression at pos %zu",
                              token_type_str(errors.invalid_token.token.type),
                              errors.invalid_token.token.pos);
        case PARSE_RESULT_TYPE_EXPECTED_FUNCTION_ARGUMENT_LIST:
            return str_format("token %s at %zu is not valid after the function "
                              "identifier, expected identifier or ')'",
                              token_type_str(errors.invalid_token.token.type),
                              errors.invalid_token.token.pos);
    }

    return to_str("INVALID RESULT TYPE");
}
