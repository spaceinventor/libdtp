src = [
    'session_hooks.c'
]
param_dep = dependency('param', fallback: ['param', 'param_dep'], required: false).partial_dependency(links: false, includes: true)

apm_utils_lib = library('apm_utils', src, include_directories: [include, include_directories('../../src')], dependencies: [dtp_lib_dep, param_dep])
apm_utils_lib_dep = declare_dependency(include_directories : include, link_with : apm_utils_lib, dependencies: [dtp_lib_dep, param_dep])
