unity = dependency('unity')

lexer_test = executable('lexer_test', 'lexer_test.c', dependencies : [unity, thor_dep])
parser_test = executable('parser_test', 'parser_test.c', dependencies : [unity, thor_dep])

test('lexer', lexer_test)
test('parser', parser_test)
