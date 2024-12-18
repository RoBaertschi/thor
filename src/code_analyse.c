#include "code_analyse.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "common.h"
#include "da.h"
#include "language.h"
#include "lexer.h"
#include "uthash.h"

// FIXME: CLEANUP IS ALL STILL MISSING, DON'T FORGET

typedef struct AnalyseData AnalyseData;
struct AnalyseData {
    Module       *m;
    Tokens       *t;
    str           input;
    ModuleAnalyse module_analyse;
    Index         cur_scope;
};

bool analyse_is_expression(Node *node) {
    switch (node->type) {
        case NODE_TYPE_INTEGER_LITERAL:
            return true;
        case NODE_TYPE_BLOCK:
        case NODE_TYPE_FUNCTION_DEFINITION:
        case NODE_TYPE_VARIABLE_DECLARATION:
        case NODE_TYPE_EOF:
            return false;
    }
    return false;
}
bool analyse_is_top_level(NodeType type) {
    switch (type) {
        case NODE_TYPE_EOF:
        case NODE_TYPE_FUNCTION_DEFINITION:
            return true;
        case NODE_TYPE_BLOCK:
        case NODE_TYPE_INTEGER_LITERAL:
        case NODE_TYPE_VARIABLE_DECLARATION:
            return false;
    }
    return false;
}

bool check_type(AnalyseData *data, Index token, Type *out_type) {
    char           *cstr = tokens_token_cstr(data->input, data->t, token);
    TypeNameToType *type;
    HASH_FIND_STR(data->module_analyse.types, cstr, type);
    if (type != NULL) {
        *out_type = type->type;
    }
    free(cstr);
    return type != NULL;
}

void begin_scope(AnalyseData *data, Index *out, Index node_index,
                 AnalyseScopeType type) {
    AnalyseScope scope = {
        .node        = node_index,
        .type        = type,
        .super_scope = data->cur_scope,
    };
    da_append(&data->module_analyse.scopes, scope);
    *out             = data->module_analyse.scopes.count - 1;

    NodeToScope *n2s = malloc(sizeof(NodeToScope));
    *n2s             = (NodeToScope){
                    .node  = node_index,
                    .scope = *out,
    };

    HASH_ADD_USZ(data->module_analyse.nodes_to_scopes, node, n2s);
}

void add_function_to_scope(AnalyseData *data, Index scope_index,
                           Node *function_node, Index function_node_index) {
    assert(function_node->type == NODE_TYPE_FUNCTION_DEFINITION);
    Index                  extra_data = function_node->data.lhs;
    FunctionPrototypeData *function_prototype =
        &data->m->extra_data.items[extra_data].data.function_prototype;

    AnalyseFunction function = {
        .node = function_node_index,
        .name =
            tokens_token_cstr(data->input, data->t, function_node->main_token),
    };

    Type return_type;
    if (!check_type(data, function_prototype->return_type, &return_type)) {
        AnalyseError error = {
            .type = ANALYSE_ERROR_UNKOWN_TYPE,
            .node = function_node_index,
        };
        da_append(&data->module_analyse.errors, error);
        return;
    }

    for (usz i = 0; i < function_prototype->args.count; i++) {
        FunctionArgument arg = function_prototype->args.items[i];
        Type             argument_type;
        if (!check_type(data, arg.type, &argument_type)) {
            AnalyseError error = {
                .type = ANALYSE_ERROR_UNKOWN_TYPE,
                .node = function_node_index,
            };
            da_append(&data->module_analyse.errors, error);
            return;
        } else {
            da_append(&function.argument_types, argument_type);
        }
    }

    AnalyseFunction *function_mem = malloc(sizeof(AnalyseFunction));
    *function_mem                 = function;

    AnalyseScope *scope = &data->module_analyse.scopes.items[scope_index];
    HASH_ADD_STR(scope->functions, name, function_mem);
}

void end_scope(AnalyseData *data, Index scope) {
    data->cur_scope = data->module_analyse.scopes.items[scope].super_scope;
}

void free_module_analyse(ModuleAnalyse *module_analyse) {

    // Node to scope
    NodeToScope *node_to_scope, *node_to_scope_tmp;
    HASH_ITER(hh, module_analyse->nodes_to_scopes, node_to_scope,
              node_to_scope_tmp) {
        HASH_DEL(module_analyse->nodes_to_scopes, node_to_scope);
        free(node_to_scope);
    }

    // Scopes
    for (usz i = 0; i < module_analyse->scopes.count; i++) {
        AnalyseScope    *scope = &module_analyse->scopes.items[i];
        AnalyseVariable *var, *var_tmp;
        HASH_ITER(hh, scope->variables, var, var_tmp) {
            HASH_DEL(scope->variables, var);
            free(var->name);
            free(var);
        }

        AnalyseFunction *func, *func_tmp;
        HASH_ITER(hh, scope->functions, func, func_tmp) {
            HASH_DEL(scope->functions, func);
            free(func->name);
            da_destroy(&func->argument_types);
            free(func);
        }
    }
    da_destroy(&module_analyse->scopes);

    // Errors
    da_destroy(&module_analyse->errors);

    // Types
    TypeNameToType *type_name_to_type, *type_name_to_type_tmp;
    HASH_ITER(hh, module_analyse->types, type_name_to_type,
              type_name_to_type_tmp) {
        HASH_DEL(module_analyse->types, type_name_to_type);
        free(type_name_to_type->type_name);
        free(type_name_to_type);
    }
}

void analyse_data_init_types(AnalyseData *analyse_data) {
    TypeNameToType *u32   = malloc(sizeof(TypeNameToType));
    u32->type.type        = BUILTIN_TYPE_U32;
    char const u32_name[] = "u32";
    u32->type_name        = malloc(sizeof(u32_name));
    memcpy(u32->type_name, u32_name, sizeof(u32_name));

    HASH_ADD_STR(analyse_data->module_analyse.types, type_name, u32);
}

// Checks recursevly, if we are in an function body
bool in_function_body(AnalyseData *analyse_data, Index scope) {
    if (analyse_data->module_analyse.root_scope == scope) {
        return false;
    }

    if (analyse_data->module_analyse.scopes.items[scope].type ==
        ANALYSE_SCOPE_TYPE_FUNCTION) {
        return true;
    }

    return in_function_body(
        analyse_data,
        analyse_data->module_analyse.scopes.items[scope].super_scope);
}

bool function_allowed_in_scope(AnalyseData *analyse_data, Index scope) {
    return !in_function_body(analyse_data, scope);
}

bool is_identifier_in_use(AnalyseData *analyse_data, Node *node, Index scope) {
    char *name = tokens_token_cstr(analyse_data->input, analyse_data->t,
                                   node->main_token);
    AnalyseFunction *func;
    HASH_FIND_STR(analyse_data->module_analyse.scopes.items[scope].functions,
                  name, func);

    AnalyseVariable *var;
    HASH_FIND_STR(analyse_data->module_analyse.scopes.items[scope].variables,
                  name, var);

    free(name);
    if (func == NULL && var == NULL) {
        if (scope == analyse_data->module_analyse.root_scope) {
            return false;
        } else {
            return is_identifier_in_use(analyse_data, node, scope);
        }
    }

    return true;
}

void analyse_node(AnalyseData *analyse_data, Node *node, Index node_index);

bool analyse_expression(AnalyseData *analyse_data, Node *node, Index node_index,
                        Type *out_type) {
    switch (node->type) {
        case NODE_TYPE_INTEGER_LITERAL:
            *out_type = (Type){.type = BUILTIN_TYPE_U32};
            return true;
        case NODE_TYPE_BLOCK:
        case NODE_TYPE_FUNCTION_DEFINITION:
        case NODE_TYPE_VARIABLE_DECLARATION:
        case NODE_TYPE_EOF:
            return false;
    }
    return false;
}

void analyse_variable(AnalyseData *analyse_data, Node *node, Index node_index) {
    assert(node->type == NODE_TYPE_VARIABLE_DECLARATION);
    if (is_identifier_in_use(analyse_data, node, analyse_data->cur_scope)) {
        AnalyseError error = {
            .node = node_index,
            .type = ANALYSE_ERROR_IDENTIFIER_ALREADY_IN_USE,
        };
        da_append(&analyse_data->module_analyse.errors, error);

        return;
    }

    Type expression_type;
    if (!analyse_expression(analyse_data, &analyse_data->m->nodes.items[node->data.rhs], node->data.rhs, &expression_type)) {
        AnalyseError error = {
            .node = node_index,
            .type = ANALYSE_ERROR_EXPECTED_EXPRESSION_FOR_VARIABLE_DECLARATION,
        };
        da_append(&analyse_data->module_analyse.errors, error);

        return;
    }

    if (node->data.lhs != 0) {
        Type variable_type;
        if (!check_type(analyse_data, node->data.lhs, &variable_type)) {
            AnalyseError error = {
                .node = node_index,
                .type = ANALYSE_ERROR_VARIABLE_UNKOWN_TYPE,
            };
            da_append(&analyse_data->module_analyse.errors, error);
            return;
        }

        if (variable_type.type != expression_type.type) {
            AnalyseError error = {
                .node = node_index,
                .type = ANALYSE_ERROR_VARIABLE_EXPRESSION_DIFFRENT_TYPE,
            };
            da_append(&analyse_data->module_analyse.errors, error);
            return;
        }
    }

    AnalyseVariable variable = {
        .type = expression_type,
        .name = tokens_token_cstr(analyse_data->input, analyse_data->t, node->main_token)
    };

    AnalyseVariable *variable_mem = malloc(sizeof(AnalyseVariable));
    *variable_mem = variable;

    HASH_ADD_STR(analyse_data->module_analyse.scopes.items[analyse_data->cur_scope].variables, name, variable_mem);
}

void analyse_block(AnalyseData *analyse_data, Node *node, Index node_index) {
    assert(node->type == NODE_TYPE_BLOCK);
    Index      extra_data = node->data.lhs;
    BlockData *block =
        &analyse_data->m->extra_data.items[extra_data].data.block;
    Index block_scope;
    begin_scope(analyse_data, &block_scope, node_index,
                ANALYSE_SCOPE_TYPE_BLOCK);

    for (usz i = 0; i < block->count; i++) {
        analyse_node(analyse_data,
                     &analyse_data->m->nodes.items[block->items[i]],
                     block->items[i]);
    }

    end_scope(analyse_data, block_scope);
}

void analyse_function_definition(AnalyseData *analyse_data, Node *node,
                                 Index node_index) {

    add_function_to_scope(analyse_data, analyse_data->cur_scope, node,
                          node_index);

    Index function_scope;
    assert(node->type == NODE_TYPE_FUNCTION_DEFINITION);
    Index                  extra_data = node->data.lhs;
    FunctionPrototypeData *function_prototype =
        &analyse_data->m->extra_data.items[extra_data].data.function_prototype;
    begin_scope(analyse_data, &function_scope, node_index,
                ANALYSE_SCOPE_TYPE_FUNCTION);

    for (usz i = 0; i < function_prototype->args.count; i++) {
        Type type;
        if (!check_type(analyse_data, function_prototype->args.items[i].type,
                        &type)) {

            AnalyseError error = {
                .type = ANALYSE_ERROR_UNKOWN_TYPE,
                .node = node_index,
            };
            da_append(&analyse_data->module_analyse.errors, error);
        }
        AnalyseVariable analyse_variable = {
            .type = type,
            .name = tokens_token_cstr(analyse_data->input, analyse_data->t,
                                      function_prototype->args.items[i].name)};
        AnalyseVariable *analyse_variable_mem = malloc(sizeof(AnalyseVariable));
        *analyse_variable_mem                 = analyse_variable;

        HASH_ADD_STR(
            analyse_data->module_analyse.scopes.items[function_scope].variables,
            name, analyse_variable_mem);
    }

    Node *block = &analyse_data->m->nodes.items[node->data.rhs];
    analyse_block(analyse_data, block, node->data.rhs);

    end_scope(analyse_data, function_scope);
}

void analyse_top_level_node(AnalyseData *analyse_data, Index node_index) {
    AnalyseError error;
    Node        *node = &analyse_data->m->nodes.items[node_index];

    switch (node->type) {
        case NODE_TYPE_FUNCTION_DEFINITION:
            analyse_function_definition(analyse_data, node, node_index);
        case NODE_TYPE_EOF:
            return;

        case NODE_TYPE_BLOCK:
        case NODE_TYPE_INTEGER_LITERAL:
        case NODE_TYPE_VARIABLE_DECLARATION:
            error = (AnalyseError){
                .type = ANALYSE_ERROR_INVALID_TOP_LEVEL_STATEMENT,
                .node = node_index,
            };
            da_append(&analyse_data->module_analyse.errors, error);
            return;
    }
}

void analyse_node(AnalyseData *analyse_data, Node *node, Index node_index) {
    AnalyseError error;
    switch (node->type) {

        case NODE_TYPE_BLOCK:
            analyse_block(analyse_data, node, node_index);
            return;
        case NODE_TYPE_FUNCTION_DEFINITION:
            if (!function_allowed_in_scope(analyse_data,
                                           analyse_data->cur_scope)) {
                AnalyseError error = {
                    .node = node_index,
                    .type = ANALYSE_ERROR_FUNCTION_NOT_ALLOWED_IN_SCOPE,
                };
                da_append(&analyse_data->module_analyse.errors, error);
                return;
            }
            analyse_function_definition(analyse_data, node, node_index);
            return;
        case NODE_TYPE_INTEGER_LITERAL:
            error = (AnalyseError){
                .node = node_index,
                .type = ANALYSE_ERROR_INVALID_NODE,
            };
            da_append(&analyse_data->module_analyse.errors, error);
            return;
        case NODE_TYPE_VARIABLE_DECLARATION:
            if (!in_function_body(analyse_data, analyse_data->cur_scope)) {
                error = (AnalyseError){
                    .type =
                        ANALYSE_ERROR_VARIABLE_NOT_ALLOWED_IN_NONE_FUNCTION_SCOPE,
                    .node = node_index,
                };
                da_append(&analyse_data->module_analyse.errors, error);
                return;
            }
            analyse_variable(analyse_data, node, node_index);
            return;
        case NODE_TYPE_EOF:
            return;
    }
}

ModuleAnalyse analyse_module(Module *m, Tokens *t, str input) {
    ModuleAnalyse module_analyse = {0};
    AnalyseData   analyse_data   = {
            .m              = m,
            .t              = t,
            .input          = input,
            .module_analyse = module_analyse,
    };

    analyse_data_init_types(&analyse_data);

    for (usz i = 0; i < m->top_level_nodes.count; i++) {
        analyse_top_level_node(&analyse_data, m->top_level_nodes.items[i]);
    }

    return module_analyse;
}
