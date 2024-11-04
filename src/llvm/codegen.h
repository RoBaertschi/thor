#pragma once

#include <llvm-c/Types.h>
#include "ast.h"
#include "lexer.h"
#include "parser.h"

typedef struct CodeGenerator CodeGenerator;
struct CodeGenerator {
    Module thor_module;
    Parser parser;
    Tokens tokens;

    LLVMContextRef context;
    LLVMBuilderRef builder;
    LLVMModuleRef module;
};

CodeGenerator code_gen_create(Tokens t, Parser p, Module m);
void code_gen_destroy(CodeGenerator cg);
