#include "code_analyse.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "common.h"
#include "da.h"
#include "language.h"
#include "lexer.h"
#include "uthash.h"

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

bool check_type(AnalyseData *data, Index token) {
    char           *cstr = tokens_token_cstr(data->input, data->t, token);
    TypeNameToType *type;
    HASH_FIND_STR(data->module_analyse.types, cstr, type);
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

void add_function_to_scope(AnalyseData *data, Index scope,
                           Node *function_node) {
    assert(function_node->type == NODE_TYPE_FUNCTION_DEFINITION);
}

void end_scope(AnalyseData *data, Index scope) {
    data->cur_scope = data->module_analyse.scopes.items[scope].super_scope;
}

void free_all_scopes(ModuleAnalyse *module_analyse) {
    NodeToScope *scope, *tmp;
    HASH_ITER(hh, module_analyse->nodes_to_scopes, scope, tmp) {
        HASH_DEL(module_analyse->nodes_to_scopes, scope);
        free(scope);
    }
}

void init_types(AnalyseData *analyse_data) {
    TypeNameToType *u32   = malloc(sizeof(TypeNameToType));
    u32->type.type        = BUILTIN_TYPE_U32;
    char const u32_name[] = "u32";
    u32->type_name        = malloc(sizeof(u32_name));
    memcpy(u32->type_name, u32_name, sizeof(u32_name));

    HASH_ADD_STR(analyse_data->module_analyse.types, type_name, u32);
}

void analyse_top_level_node(AnalyseData *analyse_data, Index node_index) {
    Node *node = &analyse_data->m->nodes.items[node_index];
    if (!analyse_is_top_level(node->type)) {
        AnalyseError error = {
            .type = ANALYSE_ERROR_INVALID_TOP_LEVEL_STATEMENT,
            .node = node_index,
        };
        da_append(&analyse_data->module_analyse.errors, error);
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

    init_types(&analyse_data);

    for (usz i = 0; i < m->top_level_nodes.count; i++) {
        analyse_top_level_node(&analyse_data, m->top_level_nodes.items[i]);
    }

    return module_analyse;
}
