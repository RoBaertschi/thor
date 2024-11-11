#pragma once

#include <llvm-c/Types.h>
#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include "uthash.h"

typedef struct NamedVariable NamedVariable;
struct NamedVariable {
    char const    *name;
    LLVMValueRef   value;
    UT_hash_handle hh;
};

typedef struct CodeGenerator CodeGenerator;
struct CodeGenerator {
    Module         thor_module;
    Parser         parser;
    Tokens         tokens;

    LLVMContextRef context;
    LLVMBuilderRef builder;
    LLVMModuleRef  module;

    NamedVariable *named_variables;
};

CodeGenerator code_gen_create(Tokens t, Parser p, Module m);
void          code_gen_destroy(CodeGenerator cg);
void          code_gen(CodeGenerator *cg);
