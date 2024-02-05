project('cspftp', 'c', subproject_dir: 'extern', version: '0.0.1')

src = [
    'src/cspftp.c'
]
include = include_directories('include')

csp_dep = dependency('csp', fallback: ['csp', 'csp_dep'], required: true)

cspftp_lib = library('cspftp', src, include_directories: [include], dependencies: [csp_dep])

unity_dep = dependency('unity', fallback: ['unity', 'unity_dep'], required: true)
utests_src = ['tests/test_1.c', 'tests/unity_test_utils.c']
utests = executable('utests', utests_src, link_with:[cspftp_lib], include_directories: [include], dependencies: [unity_dep])
test('utests', utests)

doc_out = meson.current_build_dir() / '@OUTPUT@'
doc = custom_target('doc',
  output : 'CSP_FTP.pdf',
  input : 'doc/cspftp.rst',
  command : ['gen-sidoc', '-t', 'MAN', 'CSP_FTP', '@INPUT@', '-o', doc_out]
)
