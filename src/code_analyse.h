#pragma once

#include "ast.h"
#include "language.h"
#include "lexer.h"
#include "uthash.h"

enum AnalyseScopeType {
    // Can contain functions but no variables
    ANALYSE_SCOPE_TYPE_TOP_LEVEL,
    // Cannot contain functions but can contain variables
    ANALYSE_SCOPE_TYPE_FUNCTION,
    // Same as function, just diffrent name
    ANALYSE_SCOPE_TYPE_BLOCK,
};
typedef enum AnalyseScopeType AnalyseScopeType;

enum AnalyseErrorType {
    ANALYSE_ERROR_NONE,
    ANALYSE_ERROR_INVALID_TOP_LEVEL_STATEMENT,
    ANALYSE_ERROR_FUNCTION_NOT_ALLOWED_IN_SCOPE,
    ANALYSE_ERROR_UNKOWN_TYPE,
    ANALYSE_ERROR_VARIABLE_UNKOWN_TYPE,
    ANALYSE_ERROR_VARIABLE_EXPRESSION_DIFFRENT_TYPE,
    ANALYSE_ERROR_EXPECTED_EXPRESSION_FOR_VARIABLE_DECLARATION,
    ANALYSE_ERROR_INVALID_NODE,
    ANALYSE_ERROR_VARIABLE_NOT_ALLOWED_IN_NONE_FUNCTION_SCOPE,
    ANALYSE_ERROR_IDENTIFIER_ALREADY_IN_USE,
};
typedef enum AnalyseErrorType AnalyseErrorType;

typedef struct AnalyseError   AnalyseError;
struct AnalyseError {
    AnalyseErrorType type;

    // Offending node
    Index            node;
};

typedef struct AnalyseFunction AnalyseFunction;
struct AnalyseFunction {
    UT_hash_handle hh;
    char          *name;
    Index          node;
    Type           return_type;
    struct {
        usz   count;
        usz   capacity;
        Type *items;
    } argument_types;
};

typedef struct AnalyseVariable AnalyseVariable;
struct AnalyseVariable {
    UT_hash_handle hh;
    char          *name;
    Type           type;
};

typedef struct AnalyseScope AnalyseScope;
struct AnalyseScope {
    AnalyseFunction *functions;
    AnalyseVariable *variables;

    Index            super_scope;
    // The corresponding node, function node for function, 0 for top  levels
    Index            node;
    AnalyseScopeType type;
};

typedef struct NodeToScope NodeToScope;
struct NodeToScope {
    UT_hash_handle hh;
    Index          node;
    Index          scope;
};

typedef struct TypeNameToType TypeNameToType;
struct TypeNameToType {
    UT_hash_handle hh;
    char          *type_name;
    Type           type;
};

typedef struct ModuleAnalyse ModuleAnalyse;
struct ModuleAnalyse {
    // Which scope to which node
    // All nodes are allocated with malloc, don't forget to free all of them.
    NodeToScope    *nodes_to_scopes;
    TypeNameToType *types;
    Index           root_scope;
    struct {
        usz           count;
        usz           capacity;
        AnalyseScope *items;
    } scopes;

    struct {
        usz           count;
        usz           capacity;
        AnalyseError *items;
    } errors;
};

ModuleAnalyse analyse_module(Module *m, Tokens *t, str input);
