#include "ast_walker.h"
#include "ast.h"
#include "lexer.h"

#define SAFE_CALLBACK_CALL(function, ...) \
    ((function) != NULL) ? (function)(__VA_ARGS__) : true

void ast_walker_walk_block(AstWalker *aw, Node *node) {
    Index      ed_idx = node->data.lhs;
    BlockData *bd     = &aw->m->extra_data.items[ed_idx].data.block;
    Block      block  = {.main_token = node->main_token, .bd = bd};
    bool       continue_ =
        SAFE_CALLBACK_CALL(aw->block, aw->user_data, aw, block);

    if (!continue_) {
        return;
    }

    for (usz i = 0; i < bd->count; i++) {
        Node *block_node = &aw->m->nodes.items[bd->items[i]];
        ast_walker_walk_node(aw, block_node);
    }
}
void ast_walker_walk_function_definition(AstWalker *aw, Node *node) {
    Index                  ed_idx = node->data.lhs;
    FunctionPrototypeData *fpd =
        &aw->m->extra_data.items[ed_idx].data.function_prototype;
    FunctionDefinition fd = {
        .block = node->data.rhs, .main_token = node->main_token, .fpd = fpd};
    bool continue_ = SAFE_CALLBACK_CALL(aw->function_definition, aw->user_data,
                                        aw, fd);

    if (!continue_) {
        return;
    }

    Node *block_node = &aw->m->nodes.items[node->data.rhs];
    ast_walker_walk_block(aw, block_node);
}
void ast_walker_walk_integer(AstWalker *aw, Node *node) {
    TokenExtraData *ted     = &aw->t->extra_data.data[node->main_token];
    int             literal = ted->data.integer;
    IntegerLiteral  il = {.integer = literal, .main_token = node->main_token};

    SAFE_CALLBACK_CALL(aw->integer_literal, aw->user_data, aw, il);
}
void ast_walker_walk_variable_declaration(AstWalker *aw, Node *node) {
    VariableDeclaration vd = {.main_token = node->main_token,
                              .type       = node->data.lhs,
                              .expr       = node->data.rhs};
    bool continue_ = SAFE_CALLBACK_CALL(aw->variable_declaration, aw->user_data,
                                        aw, vd);

    if (!continue_) {
        return;
    }

    Node *expr_node = &aw->m->nodes.items[node->data.rhs];
    ast_walker_walk_node(aw, expr_node);
}
void ast_walker_walk_eof(AstWalker *aw, Node *node) {
    Eof eof = {.main_token = node->main_token};

    SAFE_CALLBACK_CALL(aw->eof, aw->user_data, aw, eof);
}

void ast_walker_walk_node(AstWalker *aw, Node *node) {
    switch (node->type) {

        case NODE_TYPE_BLOCK:
            ast_walker_walk_block(aw, node);
            return;
        case NODE_TYPE_FUNCTION_DEFINITION:
            ast_walker_walk_function_definition(aw, node);
            return;
        case NODE_TYPE_INTEGER_LITERAL:
            ast_walker_walk_integer(aw, node);
            break;
        case NODE_TYPE_VARIABLE_DECLARATION:
            ast_walker_walk_variable_declaration(aw, node);
            break;
        case NODE_TYPE_EOF:
            ast_walker_walk_eof(aw, node);
            break;
    }
}

void ast_walker_walk_top_level_node(AstWalker *aw, usz top_level_node) {
    Index node_idx = aw->m->top_level_nodes.items[top_level_node];
    Node *node     = &aw->m->nodes.items[node_idx];

    ast_walker_walk_node(aw, node);
}

void ast_walker_walk(AstWalker *aw) {
    for (usz i = 0; i < aw->m->top_level_nodes.count; i++) {
        ast_walker_walk_top_level_node(aw, i);
    }
}
