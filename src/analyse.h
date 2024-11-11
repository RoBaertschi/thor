#pragma once

#include <stdbool.h>
#include "ast.h"
#include "language.h"
#include "uthash.h"

enum AnalyseError {
    ANALYSE_ERROR_NONE,
    ANALYSE_ERROR_INVALID_TOP_LEVEL_STATEMENT,
    ANALYSE_ERROR_EXPECTED_EXPRESSION_FOR_VARIABLE_DECLARATION,
    ANALYSE_ERROR_UNKOWN_TYPE,
};
typedef enum AnalyseError AnalyseError;

enum AnalyseScopeType {
    // Can contain functions but no variables
    ANALYSE_SCOPE_TYPE_TOP_LEVEL,
    // Cannot contain functions but can contain variables
    ANALYSE_SCOPE_TYPE_FUNCTION,
};
typedef enum AnalyseScopeType      AnalyseScopeType;

typedef struct AnalyseVariableData AnalyseVariableData;
struct AnalyseVariableData {
    UT_hash_handle hh;
    char          *name;
    // TODO: Custom types, make this an uint32_t and anything that is not in the
    // enum is a custom type id
    BuiltinType    type;
};

typedef struct AnalyseScope AnalyseScope;
struct AnalyseScope {
    AnalyseScopeType            type;

    // ut hash maps
    struct AnalyseFunctionData *functions;
    AnalyseVariableData        *variables;
    Index                       super_scope;
};

typedef struct AnalyseFunctionData AnalyseFunctionData;
struct AnalyseFunctionData {
    UT_hash_handle hh;
    // Key
    char          *name;
    Index          scope;
};

typedef struct AnalyseData AnalyseData;
struct AnalyseData {
    struct {
        usz           count;
        usz           capacity;
        AnalyseScope *items;
    } scopes;
    // Probably 0
    Index root_scope;
};

typedef struct AnalyseResult AnalyseResult;
struct AnalyseResult {
    // Offending node on error.
    Node        *node;
    AnalyseError type;
    bool         ok;
    AnalyseData  data;
};

bool          analy_is_expression(Node *node);
bool          analy_is_top_level(NodeType type);
AnalyseResult analyse_module(Module *module, Tokens *tokens, str *input);
