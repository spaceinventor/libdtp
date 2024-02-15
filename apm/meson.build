apm_csh_dep = dependency('apm_csh', fallback: ['apm_csh', 'apm_csh_dep'], required: true)
slash_dep = subproject('slash').get_variable('slash_dep')
apm_deps = [apm_csh_dep, slash_dep]

subdir('dtp_client')
subdir('dtp_server')