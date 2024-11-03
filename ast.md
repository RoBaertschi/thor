# AST

name := 1

VariableDeclaration(Identifier, Expression)

```c

enum StatementType {
    VARIABLE_DEFINITION
};

struct Statement {
    
};

struct Module {
    str name; // main
    vec(Statement) body;
};

```
