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

Lexer setup_lexer(const char *input) {
    str   input_str = to_str(input);
    Lexer l         = lexer_create(input_str, NULL);
    str_destroy(input_str);
    return l;
}

void expect_variable_declaration(void) {}

void test_parser_variable_decleration(void) {
    Lexer l = setup_lexer("hello : hello = 3\n");
    Tokens t = lexer_lex_tokens(&l);
    Parser p = parser_create(t, str_clone(l.input));
    ParseModuleResult mod = parser_parse_module(&p);
    if (mod.type != PARSE_RESULT_TYPE_OK) {
        str err = parse_error_str(mod.type, mod.data.errors);
        char* cerr = to_cstr(err);
        printf("%s", cerr);
        str_destroy(err);
    }
    Module m = mod.data.ok;
    print_module(&p, &m);
    TEST_ASSERT_EQUAL_size_t_MESSAGE(2, m.nodes.len, "nodes len");
    TEST_ASSERT_EQUAL_size_t(NODE_TYPE_VARIABLE_DECLARATION, m.nodes.data[1].type);
    TEST_ASSERT_EQUAL_size_t(1, m.nodes.data[1].main_token);

    parser_destroy(p);
    lexer_destroy(l);
    module_destroy(m);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_parser_variable_decleration);
    return UNITY_END();
}
