srcs = ['thor.c']
library_srcs = ['lexer.c', 'common.c', 'token.c', 'parser.c', 'ast_walker.c', 'analyse.c', 'llvm/codegen.c']

thor = library('thor', library_srcs, install : true, dependencies : [llvm_dep])
thor_includedir = include_directories('.')

thor_dep = declare_dependency(link_with : thor, include_directories : [thor_includedir])

thorc = executable('thorc', srcs,
  install : true, dependencies : [llvm_dep, thor_dep])
