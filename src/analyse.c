#include "analyse.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "ast_walker.h"
#include "common.h"
#include "da.h"
#include "language.h"
#include "lexer.h"
#include "uthash.h"

typedef struct BuiltinTypeH BuiltinTypeH;
struct BuiltinTypeH {
    UT_hash_handle hh;
    char const    *key;
    BuiltinType    value;
};

typedef struct AnalyWalkerData AnalyWalkerData;
struct AnalyWalkerData {
    struct {
        usz            count;
        usz            capacity;
        AnalyseResult *items;
    } results;
    struct {
        usz           count;
        usz           capacity;
        AnalyseScope *items;
    } scopes;
    BuiltinTypeH *builtin_types;
    BuiltinTypeH *builtin_types_memory;
    Index         root_scope;
    Index         current_scope;
};

void init_builtin_types(AnalyWalkerData *data) {
    BuiltinTypeH types[] = {
        {.key = "u32", .value = BUILTIN_TYPE_U32}
    };

    data->builtin_types_memory = malloc(sizeof(types));
    if (data->builtin_types_memory == NULL) {
        return;
    }

    memcpy(data->builtin_types_memory, types, sizeof(types));

    for (usz i = 0; i < sizeof(types) / sizeof(BuiltinTypeH); i++) {
        HASH_ADD_STR(data->builtin_types, key, &data->builtin_types_memory[i]);
    }
}

void destroy_builtin_types(AnalyWalkerData *data) {
    HASH_CLEAR(hh, data->builtin_types);
    free(data->builtin_types_memory);
}

Index insert_new_scope(AnalyWalkerData *wdata, AnalyseScope scope) {
    da_append(&wdata->scopes, scope);
    return wdata->scopes.count - 1;
}

// Expression have a value, statements don't
bool analy_is_expression(Node *node) {
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

bool analy_is_top_level(NodeType type) {
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

AnalyseResult check_type(AnalyWalkerData *data, AstWalker *awd, Index index) {
    str   token      = tokens_token_str(*awd->input, awd->t, index);
    char *token_cstr = to_cstr(token);
    str_destroy(token);
    BuiltinTypeH *builtin_type;
    HASH_FIND_STR(data->builtin_types, token_cstr, builtin_type);
    free(token_cstr);

    if (builtin_type == NULL) {
        AnalyseResult result = {.type = ANALYSE_ERROR_UNKOWN_TYPE,
                                .ok   = false,
                                .node = &awd->m->nodes.items[index]};
        return result;
    }

    return (AnalyseResult){.ok = true};
}

bool walk_variable_declaration(void *data, AstWalker *awd,
                               VariableDeclaration vd) {
    AnalyWalkerData *wdata = data;

    if (vd.type != 0) {
        AnalyseResult result = check_type(wdata, awd, vd.type);

        if (!result.ok) {
            da_append(&wdata->results, result);
            return false;
        }
    }

    if (!analy_is_expression(&awd->m->nodes.items[vd.expr])) {
        AnalyseResult result = {
            .type = ANALYSE_ERROR_EXPECTED_EXPRESSION_FOR_VARIABLE_DECLARATION,
            .ok   = false,
            .node = &awd->m->nodes.items[vd.expr]};
        da_append(&wdata->results, result);

        return false;
    }

    return true;
}

bool walk_function_definition(void *data, AstWalker *awd,
                              FunctionDefinition fd) {

    AnalyWalkerData *wdata       = data;
    Index            super_scope = wdata->current_scope;

    wdata->current_scope         = insert_new_scope(
        wdata, (AnalyseScope){.super_scope = super_scope,
                                      .variables   = NULL,
                                      .functions   = NULL,
                                      .type        = ANALYSE_SCOPE_TYPE_FUNCTION});
    for (usz i = 0; i < fd.fpd->args.count; i++) {
        AnalyseResult result =
            check_type(wdata, awd, fd.fpd->args.items[i].type);

        if (!result.ok) {
            da_append(&wdata->results, result);
            return false;
        }
    }

    AnalyseResult result = check_type(wdata, awd, fd.fpd->return_type);
    if (!result.ok) {
        da_append(&wdata->results, result);
        return false;
    }

    char *fname = tokens_token_cstr(*awd->input, awd->t, fd.main_token);
    AnalyseFunctionData *afd = malloc(sizeof(AnalyseFunctionData));
    *afd = (AnalyseFunctionData){.name = fname, .scope = wdata->current_scope};

    HASH_ADD_STR(wdata->scopes.items[wdata->current_scope].functions, name,
                 afd);
    ast_walker_walk_block(awd, &awd->m->nodes.items[fd.block]);

    wdata->current_scope = super_scope;

    return false;
}

AnalyseResult analyse_module(Module *module, Tokens *tokens, str *input) {
    AnalyWalkerData data = {0};
    init_builtin_types(&data);

    for (usz i = 0; i < module->top_level_nodes.count; i++) {
        Node *node = &module->nodes.items[module->top_level_nodes.items[i]];

        if (!analy_is_top_level(node->type)) {
            return (AnalyseResult){
                .type = ANALYSE_ERROR_INVALID_TOP_LEVEL_STATEMENT,
                .node = node,
                .ok   = false};
        }
    }

    AstWalker walker = {
        .m                    = module,
        .t                    = tokens,
        .input                = input,
        .user_data            = &data,
        .variable_declaration = walk_variable_declaration,
        .function_definition  = walk_function_definition,
    };

    data.root_scope = insert_new_scope(
        &data, (AnalyseScope){.type      = ANALYSE_SCOPE_TYPE_TOP_LEVEL,
                              .functions = NULL,
                              .variables = NULL});
    data.current_scope = data.root_scope;

    ast_walker_walk(&walker);

    if (data.results.count > 0) {
        return data.results.items[0];
    }
    return (AnalyseResult){.ok = true};
}
