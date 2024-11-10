#pragma once

#include <stdbool.h>
#include "ast.h"
#include "lexer.h"

typedef struct Block Block;
struct Block {
    Index      main_token;
    BlockData *bd;
};

struct AstWalker;

typedef bool (*ast_walker_block_callback)(void *data, struct AstWalker *awd,
                                          Block block);

typedef struct FunctionDefinition FunctionDefinition;
struct FunctionDefinition {
    Index                  main_token;
    FunctionPrototypeData *fpd;
    Index                  block;
};

typedef bool (*ast_walker_function_definiton_callback)(void              *data,
                                                       struct AstWalker  *awd,
                                                       FunctionDefinition fd);

typedef struct IntegerLiteral IntegerLiteral;
struct IntegerLiteral {
    Index main_token;
    int   integer;
};

typedef bool (*ast_walker_integer_literal_callback)(void             *data,
                                                    struct AstWalker *awd,
                                                    IntegerLiteral    il);

typedef struct VariableDeclaration VariableDeclaration;
struct VariableDeclaration {
    Index main_token;
    Index type;
    Index expr;
};

typedef bool (*ast_walker_variable_declaration_callback)(
    void *data, struct AstWalker *awd, VariableDeclaration vd);

typedef struct Eof Eof;
struct Eof {
    Index main_token;
};

typedef bool (*ast_walker_eof_callback)(void *data, struct AstWalker *awd,
                                        Eof eof);

typedef struct AstWalker AstWalker;
struct AstWalker {
    ast_walker_block_callback                block;
    ast_walker_function_definiton_callback   function_definition;
    ast_walker_integer_literal_callback      integer_literal;
    ast_walker_variable_declaration_callback variable_declaration;
    ast_walker_eof_callback                  eof;

    Module *m;
    Tokens *t;
    str    *input;
    void   *user_data;
};

void ast_walker_walk(AstWalker *aw);
void ast_walker_walk_top_level_node(AstWalker *aw, usz top_level_node);
void ast_walker_walk_node(AstWalker *aw, Node *node);
void ast_walker_walk_block(AstWalker *aw, Node *node);
void ast_walker_walk_function_definition(AstWalker *aw, Node *node);
void ast_walker_walk_integer(AstWalker *aw, Node *node);
void ast_walker_walk_variable_declaration(AstWalker *aw, Node *node);
void ast_walker_walk_eof(AstWalker *aw, Node *node);
