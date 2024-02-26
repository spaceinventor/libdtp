dtp_client_apm = shared_library(
    'csh_dtp_client',
    ['src/dtp_client_apm.c'],
    include_directories: [include_directories('../../../src')],
    dependencies: [dtp_client_lib_dep] + apm_deps,
    install: true
)
