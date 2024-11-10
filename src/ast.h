#pragma once

#include "common.h"
#include "lexer.h"

enum NodeExtraDataType {
    NODE_EXTRA_DATA_FUNCTION_PROTOTYPE,
    NODE_EXTRA_DATA_BLOCK,
};
typedef enum NodeExtraDataType NodeExtraDataType;

typedef struct FunctionArgument FunctionArgument;
struct FunctionArgument {
    Index name;
    Index type;
};
typedef struct FunctionArguments FunctionArguments;
struct FunctionArguments {
    usz               count;
    usz               capacity;
    FunctionArgument *items;
};

typedef struct FunctionPrototypeData FunctionPrototypeData;
struct FunctionPrototypeData {
    // Token Index
    Index             return_type;
    FunctionArguments args;
};

// Contains a list of Indexes to diffrent top level nodes.
typedef struct BlockData BlockData;
struct BlockData {
    usz    count;
    usz    capacity;
    Index *items;
};

typedef struct NodeExtraData NodeExtraData;
struct NodeExtraData {
    NodeExtraDataType type;
    union {
        FunctionPrototypeData function_prototype;
        BlockData             block;
    } data;
};

// The Index 0 is the None Token, it means that an optional element is not
// there.

#define NODE(name) NODE_TYPE_##name

enum NodeType {
    // lhs Index to the BlockData in ExtraData
    // rhs is unused
    NODE(BLOCK),
    // lhs is a Index into the extra data
    // rhs is the Index to a Block Node
    NODE(FUNCTION_DEFINITION),
    // lhs and rhs are not used
    // the main_token is the integer
    NODE(INTEGER_LITERAL),
    // name : (optional type) = rhs
    // main_token is `name`
    // lhs is the optional type.
    // rhs is the expression
    NODE(VARIABLE_DECLARATION),
    // End of file
    // main_token is the eof token
    NODE(EOF),
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
        usz   capacity;
        usz   count;
        Node *items;
    } nodes;
    // Nodes that are top_level for analysis of a ast walker.
    struct {
        usz    capacity;
        usz    count;
        Index *items;
    } top_level_nodes;

    // Nodes that are top_level for analysis of a ast walker.
    struct {
        usz            capacity;
        usz            count;
        NodeExtraData *items;
    } extra_data;
};
