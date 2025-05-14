project('status', 'c',
  meson_version: '>=1.0',
)

cc = meson.get_compiler('c')

threads = dependency('threads')

libevent = cc.find_library('event', static: true)
libutil = cc.find_library('util', static: true)

# static lib poop to make luajit link
libm = cc.find_library('m', static: true)
libcpp = cc.find_library('c++abi', static: true)

luajit = dependency('luajit')
luajit_libdir = luajit.get_variable(pkgconfig: 'libdir')
luajit_libname = luajit.get_variable(pkgconfig: 'libname')
luajit_incdir = luajit.get_variable(pkgconfig: 'includedir')
luajit_static = cc.find_library(luajit_libname, static: true, dirs: luajit_libdir)

luajit_prog = find_program('luajit')

kcgi_dep = dependency('kcgi', static: true)

genlibs = []

genlibs += static_library('event',
  'event.c',
  include_directories: [luajit_incdir],
  dependencies: [luajit_static, libevent]
)

genlibs += static_library('kcgi',
  'kcgi.c',
  include_directories: [luajit_incdir],
  dependencies: [luajit_static, kcgi_dep]
)

genlibs += static_library('openbsd',
  'openbsd.c',
  include_directories: [luajit_incdir],
  dependencies: [luajit_static],
)

liblua_openbsd = shared_library('openbsd',
  'openbsd.c',
  name_prefix: '',
  dependencies: [luajit],
)

genlibs += static_library('imsg',
  'imsg.c',
  include_directories: [luajit_incdir],
  dependencies: [luajit_static, libutil],
)

liblua_imsg = shared_library('imsg',
  'imsg.c',
  name_prefix: '',
  dependencies: [luajit, libutil],
)

# makes a symbol, luaJIT_BC_<name>.lua
ljgen = generator(luajit_prog, output: '@BASENAME@_bytecode.h',
  arguments: ['-b', '@INPUT@', '@OUTPUT@'])

bytecode_srcs = [
  '3p/template/template.lua',
  'http.lua',
  'router.lua',
  'boot.lua',
]

bytecode_headers = []

foreach b : bytecode_srcs
  bytecode_headers += ljgen.process(b)
endforeach

bytecode_c = custom_target('bytecode.c',
  output: 'bytecode.c',
  input: 'scripts' / 'genbc.lua',
  command: [luajit_prog, '@INPUT@', '@OUTPUT@', bytecode_srcs],
)

genlib_names = []
foreach g : genlibs
  genlib_names += g.name()
endforeach

genlib_c = custom_target('genlib.c',
  output: 'genlib.c',
  input: 'scripts' / 'genlib.lua',
  command: [luajit_prog, '@INPUT@', '@OUTPUT@', genlib_names],
)

kluad_srcs = [
  bytecode_c,
  genlib_c,
  'main.c',
]

kluad = executable('kluad', kluad_srcs,
  sources: bytecode_headers,
  # static binary for www chroot
  link_args: [ '-static' ],
  include_directories: [luajit_incdir],
  dependencies: [libm, libcpp, threads, kcgi_dep, luajit_static],
  link_with: genlibs,
  install: true,
  install_dir: '/var/www/cgi-bin',
)

install_data('init.lua', install_dir: '/var/www/conf')
install_data('kluad.rc', rename: 'kluad', install_dir: get_option('sysconfdir') / 'rc.d')

env = environment()

env.set('LUA_CPATH', meson.current_build_dir() / '?.so;', separator: ':')
env.set('LUA_PATH', meson.current_source_dir() / '?.lua' + ';', separator: ':')

meson.add_devenv(env)

subdir('test')

