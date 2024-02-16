unity_dep = dependency('unity', fallback: ['unity', 'unity_dep'], required: true)
unity_test_utils_lib = library('unity_test_utils', 'unity_test_utils.c', dependencies: [unity_dep])
unity_test_dep = declare_dependency(include_directories : '.', link_with : unity_test_utils_lib, dependencies: [unity_dep])

test('test_1', executable('test_1', 'test_1.c', dependencies: [cspftp_lib_dep, unity_test_dep]))

test('test_segmentation', executable('test_segmentation', 'test_segmentation.c', dependencies: [cspftp_lib_dep, unity_test_dep]))

# Integration test 1: running an intra process transfer, everything (client, server, router) runs in its own thread in the same process
test('test_integration_1', executable('test_integration_1', 'test_integration_1.c', include_directories: [], dependencies:[cspftp_lib_dep, slash_dep, unity_test_dep]))

# Integration test 2: running an inter process transfer, client, server, router all run in different processes on the same machine
test('test_integration_2', executable('test_integration_2', 'test_integration_2.c', include_directories: [], dependencies:[cspftp_lib_dep, slash_dep, unity_test_dep]))

# Integration test 3: you guessed it, an actual networked transfer, client and server on physically separated CSP nodes (one of them possibly in space), a CSH instance taking care of the routing.
# TODO: write this test ;)