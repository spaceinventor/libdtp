unity_dep = dependency('unity', fallback: ['unity', 'unity_dep'], required: true)
unity_test_utils_lib = library('unity_test_utils', 'unity_test_utils.c', dependencies: [unity_dep])
unity_test_dep = declare_dependency(include_directories : '.', link_with : unity_test_utils_lib, dependencies: [unity_dep])

test('test_1', executable('test_1', 'test_1.c', dependencies: [cspftp_lib_dep, unity_test_dep]))
test('test_integration_1', executable('test_integration_1', 'test_integration_1.c', include_directories: [private_include], dependencies:[cspftp_lib_dep]))
