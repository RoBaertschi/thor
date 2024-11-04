project(
  'thor',
  ['c', 'cpp'],
  version: '0.1',
  default_options: ['warning_level=3', 'c_std=c17'],
)

llvm_dep = dependency('llvm', version: '>=18')

subdir('src')
subdir('tests')
