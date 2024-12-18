#include <stdio.h>
#include <stdlib.h>
#include "ast.h"
#include "common.h"
#include "lexer.h"
#include "parser.h"
#include "unity.h"
#include "unity_internals.h"

void setUp(void) {}
void tearDown(void) { string_pool_free_all(); }

Lexer setup_lexer(char const *input) {
    str   input_str = to_str(input);
    Lexer l         = lexer_create(input_str, NULL);
    str_destroy(input_str);
    return l;
}

void expect_variable_declaration(void) {}

void test_parser_variable_decleration(void) {
    Lexer             l   = setup_lexer("hello : hello = 3\n");
    Tokens            t   = lexer_lex_tokens(&l);
    Parser            p   = parser_create(t, str_clone(l.input));
    ParseModuleResult mod = parser_parse_module(&p);
    if (mod.type != PARSE_RESULT_TYPE_OK) {
        str   err  = parse_error_str(mod.type, mod.data.errors);
        char *cerr = to_cstr_in_string_pool(err);
        printf("%s", cerr);
        str_destroy(err);
    }
    Module m = mod.data.ok;
    print_module(&p, &m);
    TEST_ASSERT_EQUAL_size_t_MESSAGE(2, m.nodes.count, "nodes len");
    TEST_ASSERT_EQUAL_size_t(NODE_TYPE_VARIABLE_DECLARATION,
                             m.nodes.items[1].type);
    TEST_ASSERT_EQUAL_size_t(1, m.nodes.items[1].main_token);

    parser_destroy(p);
    lexer_destroy(l);
    module_destroy(m);
}

void test_parser_function(void) {
    Lexer  l = setup_lexer("fn helloworld(test test) test {  }\n");
    Tokens t = lexer_lex_tokens(&l);
    Parser p = parser_create(t, str_clone(l.input));
    ParseModuleResult mod = parser_parse_module(&p);
    if (mod.type != PARSE_RESULT_TYPE_OK) {
        print_tokens(&t);
        str err = parse_error_str(mod.type, mod.data.errors);
        str_fprintln(stdout, err);
        str_destroy(err);
        TEST_ABORT();
        abort();
    }
    Module m = mod.data.ok;
    print_module(&p, &m);

    TEST_ASSERT_EQUAL_size_t(3, m.nodes.count);
    TEST_ASSERT_EQUAL_size_t(2, m.top_level_nodes.count);
    TEST_ASSERT_EQUAL_size_t(NODE_TYPE_BLOCK, m.nodes.items[0].type);
    Node *node = &m.nodes.items[1];
    TEST_ASSERT_EQUAL_size_t(NODE_TYPE_FUNCTION_DEFINITION, node->type);
    TEST_ASSERT_EQUAL_size_t(2, m.extra_data.count);
    NodeExtraData ned = m.extra_data.items[node->data.lhs];
    TEST_ASSERT_EQUAL_size_t(NODE_EXTRA_DATA_FUNCTION_PROTOTYPE, ned.type);

    FunctionPrototypeData fpd = ned.data.function_prototype;
    TEST_ASSERT_EQUAL_size_t(1, fpd.args.count);
    TEST_ASSERT_EQUAL_size_t(fpd.args.items[0].name, 4);
    TEST_ASSERT_EQUAL_size_t(fpd.args.items[0].type, 5);

    parser_destroy(p);
    lexer_destroy(l);
    module_destroy(m);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_parser_variable_decleration);
    RUN_TEST(test_parser_function);
    return UNITY_END();
}
