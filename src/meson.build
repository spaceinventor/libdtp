lib_src = [
    'dtp.c',
    'dtp_log.c',
    'dtp_protocol.c',
]
client_src = [
    'dtp_client.c',
    'segments_utils.c',
]
server_src = [
    'dtp_server.c',
    'payload_api.c'
]

csp_dep = subproject('csp').get_variable('csp_dep')
param_dep = subproject('param').get_variable('param_dep')

lib_deps = [csp_dep, param_dep]

dtp_lib = library('dtp', lib_src, include_directories: [include, include_directories('.')], dependencies: lib_deps)
dtp_lib_dep = declare_dependency(include_directories : include, link_with : dtp_lib, dependencies: lib_deps)

dtp_client_lib = library('dtpclient', client_src, include_directories: [include, include_directories('.')], dependencies: dtp_lib_dep)
dtp_client_lib_dep = declare_dependency(include_directories : include, link_with : dtp_client_lib, dependencies: dtp_lib_dep)

dtp_server_lib = library('dtpserver', server_src, include_directories: [include, include_directories('.')], dependencies: dtp_lib_dep)
dtp_server_lib_dep = declare_dependency(include_directories : include, link_with : dtp_server_lib, dependencies: dtp_lib_dep)
