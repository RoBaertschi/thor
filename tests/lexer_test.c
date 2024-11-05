#include "common.h"
#include "lexer.h"
#include "token.h"
#include "unity.h"
#include "unity_internals.h"

void setUp(void) {}
void tearDown(void) { string_pool_free_all(); }

void expect_identifier(Lexer *l, Tokens *t, Index i, char const *expected) {

    TEST_ASSERT_EQUAL(TOKEN_TYPE_IDENTIFIER, t->tokens[i].type);

    str expected_str = to_str(expected);
    str got = to_strl(l->input.ptr + t->tokens[i].pos, t->tokens[i].len);

    TEST_ASSERT_MESSAGE(str_equal(expected_str, got),
                        "Expected expected_str to equal got");

    str_destroy(got);
    str_destroy(expected_str);
}

void expect_integer(Tokens *t, Index i, int expected) {
    TEST_ASSERT_EQUAL(TOKEN_TYPE_INTEGER, t->tokens[i].type);
    TEST_ASSERT_EQUAL_INT(expected,
                          extra_data_integer(t, t->tokens[i].extra_data));
}

void expect_type(Tokens *t, Index i, TokenType expected) {
    TEST_ASSERT_EQUAL(expected, t->tokens[i].type);
}

void lexer_test_identifier(void) {
    // NONE, IDENT, EOF
    str    ident = to_str("hello");
    Lexer  l     = lexer_create(ident, NULL);
    Tokens t     = lexer_lex_tokens(&l);

    TEST_ASSERT_EQUAL_size_t_MESSAGE(3, t.len, "Length");
    expect_identifier(&l, &t, 1, "hello");

    str_destroy(ident);
    tokens_destroy(t);
    lexer_destroy(l);
}

void lexer_test_integer(void) {
    str    str = to_str("22");
    Lexer  l   = lexer_create(str, NULL);
    Tokens t   = lexer_lex_tokens(&l);

    TEST_ASSERT_EQUAL_size_t(3, t.len);
    expect_integer(&t, 1, 22);

    str_destroy(str);
    tokens_destroy(t);
    lexer_destroy(l);
}

void lexer_test_var(void) {
    str    str = to_str("hello := 3\n");
    Lexer  l   = lexer_create(str, NULL);
    Tokens t   = lexer_lex_tokens(&l);

    TEST_ASSERT_EQUAL_size_t(7, t.len);
    expect_identifier(&l, &t, 1, "hello");
    expect_type(&t, 2, TOKEN_TYPE_COLON);
    expect_type(&t, 3, TOKEN_TYPE_EQUAL);
    expect_integer(&t, 4, 3);
    expect_type(&t, 5, TOKEN_TYPE_EOL);
    expect_type(&t, 6, TOKEN_TYPE_EOF);

    str_destroy(str);
    tokens_destroy(t);
    lexer_destroy(l);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(lexer_test_identifier);
    RUN_TEST(lexer_test_integer);
    RUN_TEST(lexer_test_var);
    return UNITY_END();
}
