project('dtp', 'c', subproject_dir: 'extern', version: '0.0.1', default_options: [
  'default_library=static',
	'b_lto=false',
  'b_lundef=false',
])

include = include_directories('include')
install_subdir('include', install_dir : '.')

subdir('src')

gen_doc = find_program('gen-sidoc', required: false)

if gen_doc.found()
  doc_out = meson.current_build_dir() / '@OUTPUT@'
  doc = custom_target('doc',
    output : 'DTP.pdf',
    input : ['doc/dtp.rst', 'README.rst'],
    command : ['gen-sidoc', '-t', 'MAN', 'DTP', '@INPUT0@', '-o', doc_out],
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

dtp_client_dep = declare_dependency(include_directories : include, link_with : dtp_client_lib, dependencies: dtp_lib_dep)
dtp_server_dep = declare_dependency(include_directories : include, link_with : dtp_server_lib, dependencies: dtp_lib_dep)

if not meson.is_subproject()
    subdir('tests')
    pkg = import('pkgconfig')
    pkg.generate(
      name: meson.project_name(),
      version: meson.project_version(),
      libraries: [dtp_client_lib, dtp_server_lib],
      description: 'Data Transfer Protocol over CSP'
    )
endif
