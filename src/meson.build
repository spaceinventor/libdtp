lib_src = [
    'dtp.c',
    'dtp_log.c',
    'dtp_protocol.c',
    'dtp_os_hal.c',
]
client_src = [
    'dtp_client_posix_session_hooks.c',
    'dtp_client_posix_segments_utils.c',
    'dtp_client.c',
    'dtp_vmem_client.c',
]
server_src = [
    'dtp_server.c',
    'dtp_file_payload.c',
    'dtp_vmem_server.c',
    'payload_api.c'
]

csp_dep = dependency('csp', fallback:['csp', 'csp_dep'])
param_dep = dependency('param', fallback:['param', 'param_dep'])
ossi_dep = dependency('ossi', fallback:['ossi', 'ossi_dep'])

lib_deps = [csp_dep, param_dep, ossi_dep]

dtp_lib = library('dtp', lib_src, include_directories: [include, include_directories('.')], dependencies: lib_deps, install: true, implicit_include_directories: false)
dtp_lib_dep = declare_dependency(include_directories : include, link_with : dtp_lib, dependencies: lib_deps)

dtp_client_lib = library('dtp_client', client_src, include_directories: [include, include_directories('.')], dependencies: dtp_lib_dep, install: true, implicit_include_directories: false)
dtp_client_lib_dep = declare_dependency(include_directories : include, link_with : dtp_client_lib, dependencies: dtp_lib_dep)

dtp_server_lib = library('dtp_server', server_src, include_directories: [include, include_directories('.')], dependencies: dtp_lib_dep, install: true, implicit_include_directories: false)
dtp_server_lib_dep = declare_dependency(include_directories : include, link_with : dtp_server_lib, dependencies: dtp_lib_dep)
