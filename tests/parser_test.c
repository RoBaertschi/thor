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
    Lexer l = setup_lexer("hello : = 3\n");
    Tokens t = lexer_lex_tokens(&l);
    print_tokens(&t);
    Parser p = parser_create(t, str_clone(l.input));
    ParseModuleResult mod = parser_parse_module(&p);
    TEST_ASSERT_EQUAL_MESSAGE(PARSE_RESULT_TYPE_OK, mod.type, "parse module");
    Module m = mod.data.ok;

    TEST_ASSERT_EQUAL(2, m.nodes.len);
    TEST_ASSERT_EQUAL(NODE_TYPE_VARIABLE_DECLARATION, m.nodes.data[1].type);
    TEST_ASSERT_EQUAL(1, m.nodes.data[1].main_token);

    parser_destroy(p);
    lexer_destroy(l);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_parser_variable_decleration);
    return UNITY_END();
}
