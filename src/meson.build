# private_include = 

src = [
    'cspftp.c',
    'cspftp_log.c',
    'cspftp_protocol.c',
    'cspftp_server.c',
    'cspftp_client.c',
]

csp_dep = subproject('csp').get_variable('csp_dep')
param_dep = subproject('param').get_variable('param_dep')

lib_deps = [csp_dep, param_dep]
cspftp_lib = library('cspftp', src, include_directories: [include, include_directories('.')], dependencies: lib_deps)
cspftp_lib_dep = declare_dependency(include_directories : include, link_with : cspftp_lib, dependencies: lib_deps)
