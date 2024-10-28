project('thor', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3',
                     'cpp_std=c++14'])


srcs = ['thor.cpp', 'utils.cpp']

exe = executable('thor', srcs,
  install : true)

test('basic', exe)
