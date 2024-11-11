#pragma once

enum BuiltinType {
    BUILTIN_TYPE_U32,
};

typedef enum BuiltinType BuiltinType;

typedef struct Type Type;
struct Type {
    BuiltinType type;
};
