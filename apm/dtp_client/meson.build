# apm_csh_dep = dependency('apm_csh', fallback: ['apm_csh', 'apm_csh_dep'], required: true)
dtp_client_apm = shared_library('csh_dtp_client', ['src/dtp_client_apm.c'], dependencies: [cspftp_lib_dep, apm_csh_dep], install: true)
