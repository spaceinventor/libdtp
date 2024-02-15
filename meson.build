project('cspftp', 'c', subproject_dir: 'extern', version: '0.0.1', default_options: [])
# 	'csp:packet_padding_bytes=42', 
# 	'csp:buffer_count=1000',
# #	'csp:buffer_size=2048',
# 	'csp:conn_max=20',
# 	'csp:conn_rxqueue_len=1000',
# 	'csp:qfifo_len=1000',
# 	'csp:rdp_max_window=1000',
# 	'csp:port_max_bind=16',
# 	'csp:use_rtable=true',
#   'param:list_dynamic=true',
#   'default_library=static'])

include = include_directories('include')

subdir('src')

# src = [
#     'src/cspftp.c',
#     'src/cspftp_log.c',
#     'src/cspftp_protocol.c',
#     'src/cspftp_server.c',
#     'src/cspftp_client.c',
#     'src/session_data.c'
# ]
# include = include_directories('include')

# private_include = include_directories('src')

# csp_dep = subproject('csp').get_variable('csp_dep')
# param_dep = subproject('param').get_variable('param_dep')

# lib_deps = [csp_dep, param_dep]
# cspftp_lib = library('cspftp', src, include_directories: [include, private_include], dependencies: lib_deps)
# cspftp_lib_dep = declare_dependency(include_directories : include, link_with : cspftp_lib, dependencies: lib_deps)


# subdir('apm')
# subdir('tests')

gen_doc = find_program('gen-sidoc', required: false)

if gen_doc.found()
  doc_out = meson.current_build_dir() / '@OUTPUT@'
  doc = custom_target('doc',
    output : 'CSP_FTP.pdf',
    input : ['doc/cspftp.rst', 'README.rst'],
    command : ['gen-sidoc', '-t', 'MAN', 'CSP_FTP', '@INPUT0@', '-o', doc_out],
    build_by_default: false
  )
endif

doxygen = find_program('doxygen', required : false)
if doxygen.found()
  cdata = configuration_data()
  cdata.set('VERSION', meson.project_version())

  if find_program('dot', required : false).found()
    cdata.set('HAVE_DOT', 'YES')
  else
    cdata.set('HAVE_DOT', 'NO')
  endif

  cdata.set('TOP_SRCDIR', meson.project_source_root())
  cdata.set('TOP_BLDDIR', meson.project_build_root())

  doxyfile = configure_file(input: 'Doxyfile.in',
                            output: 'Doxyfile',
                            configuration: cdata,
                            install: false)

  html_target = custom_target('api-docs',
                              input: doxyfile,
                              output: 'html',
                              command: [doxygen, doxyfile],
                              build_by_default: false)
endif