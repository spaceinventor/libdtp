project('cspftp', 'c', subproject_dir: 'extern', version: '0.0.1', default_options: ['default_library=static'])

src = [
    'src/cspftp.c'
]
include = include_directories('include')

csp_dep = dependency('csp', fallback: ['csp', 'csp_dep'], required: true)
param_dep = dependency('param', fallback: ['param', 'param_dep'], required: true)

cspftp_lib = library('cspftp', src, include_directories: [include], dependencies: [csp_dep, param_dep])
cspftp_lib_dep = declare_dependency(include_directories : include, link_with : cspftp_lib, dependencies: [csp_dep, param_dep])

unity_dep = dependency('unity', fallback: ['unity', 'unity_dep'], required: true)
utests_src = ['tests/test_1.c', 'tests/unity_test_utils.c']
utests = executable('utests', utests_src, dependencies: [cspftp_lib_dep, unity_dep])
test('utests', utests)

gen_doc = find_program('gen-sidoc', required: false)

if gen_doc.found()
  doc_out = meson.current_build_dir() / '@OUTPUT@'
  doc = custom_target('doc',
    output : 'CSP_FTP.pdf',
    input : 'doc/cspftp.rst',
    command : ['gen-sidoc', '-t', 'MAN', 'CSP_FTP', '@INPUT@', '-o', doc_out]
  )
endif