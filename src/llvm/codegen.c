#include "codegen.h"
#include <assert.h>
#include <llvm-c/Core.h>
#include <llvm-c/Types.h>
#include <stdio.h>
#include <stdlib.h>
#include "ast.h"
#include "common.h"
#include "lexer.h"
#include "parser.h"

CodeGenerator code_gen_create(Tokens t, Parser p, Module m) {
    LLVMContextRef context     = LLVMContextCreate();
    LLVMBuilderRef builder     = LLVMCreateBuilderInContext(context);
    char          *module_name = to_cstr(m.name);
    LLVMModuleRef  module =
        LLVMModuleCreateWithNameInContext(module_name, context);
    free(module_name);

    return (CodeGenerator){
        .thor_module = m,
        .parser      = p,
        .tokens      = t,

        .context = context,
        .builder = builder,
        .module  = module,

        .named_variables = NULL,
    };
}

void code_gen_destroy(CodeGenerator cg) {
    LLVMDisposeModule(cg.module);
    LLVMDisposeBuilder(cg.builder);
    LLVMContextDispose(cg.context);

    module_destroy(cg.thor_module);
    parser_destroy(cg.parser);
    tokens_destroy(cg.tokens);
}

void cg_top_level(CodeGenerator *cg, Node *node);

void cg_module(CodeGenerator *cg) {
    for (usz i = 0; i < cg->thor_module.top_level_nodes.count; i++) {
        cg_top_level(
            cg,
            cg->thor_module.nodes.items[cg->thor_module.top_level_nodes[i]]);
    }
}

void cg_top_level(CodeGenerator *cg, Node *node) {}

LLVMValueRef cg_integer_literal(CodeGenerator *cg, Node *node) {
    int integer = extra_data_integer(
        &cg->tokens, cg->tokens.tokens[node->main_token].extra_data);
    return LLVMConstInt(LLVMInt32TypeInContext(cg->context), integer, false);
}

LLVMValueRef cg_get_value(CodeGenerator *cg, Node *node) {
    assert(is_expression(node) && "node is not an expression");

    switch (node->type) {
        case NODE_TYPE_INTEGER_LITERAL:
            return cg_integer_literal(cg, node);

        case NODE_TYPE_BLOCK:
        case NODE_TYPE_FUNCTION_DEFINITION:
        case NODE_TYPE_VARIABLE_DECLARATION:
        case NODE_TYPE_EOF:
            fprintf(stderr, "not a valid expression");
            abort();
    }
}

// Expression have a value, statements don't
bool is_expression(Node *node) {
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
