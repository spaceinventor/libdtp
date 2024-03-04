lib_src = [
    'cspftp.c',
    'cspftp_log.c',
    'cspftp_protocol.c',
]
client_src = [
    'cspftp_client.c',
    'segments_utils.c',
]
server_src = [
    'cspftp_server.c',
    'payload_api.c'
]

csp_dep = subproject('csp').get_variable('csp_dep')
param_dep = subproject('param').get_variable('param_dep')

lib_deps = [csp_dep, param_dep]

cspftp_lib = library('cspftp', lib_src, include_directories: [include, include_directories('.')], dependencies: lib_deps)
cspftp_lib_dep = declare_dependency(include_directories : include, link_with : cspftp_lib, dependencies: lib_deps)

dtp_client_lib = library('dtpclient', client_src, include_directories: [include, include_directories('.')], dependencies: cspftp_lib_dep)
dtp_client_lib_dep = declare_dependency(include_directories : include, link_with : dtp_client_lib, dependencies: cspftp_lib_dep)

dtp_server_lib = library('dtpserver', server_src, include_directories: [include, include_directories('.')], dependencies: cspftp_lib_dep)
dtp_server_lib_dep = declare_dependency(include_directories : include, link_with : dtp_server_lib, dependencies: cspftp_lib_dep)
