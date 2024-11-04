#pragma once

#include "common.h"
#include "lexer.h"

// The TokenIndex 0 is the None Token, it means that an optional element is not
// there.

#define NODE(name) NODE_TYPE_##name

enum NodeType {
    // name : (optional type) = rhs
    // main_token is `name`
    // lhs is the optional type.
    // rhs is the expression
    NODE(VARIABLE_DECLARATION),
    // lhs and rhs are not used
    // the main_token is the integer
    NODE(INTEGER_LITERAL),
};
typedef enum NodeType NodeType;

typedef struct NodeData NodeData;
struct NodeData {
    Index lhs;
    Index rhs;
};

typedef struct Node Node;
struct Node {
    NodeType type;
    Index    main_token;
    NodeData data;
};

typedef struct Module Module;
struct Module {
    str name;
    struct {
        usz   cap;
        usz   len;
        Node *data;
    } nodes;
    // Nodes that are top_level for analysis of a ast walker.
    struct {
        usz    cap;
        usz    len;
        Index *data;
    } top_level_nodes;
};
