project('connman-gtk', 'c',
	version : '1.1.1',
	license: [
        'GPL-2.0',
         ],
         default_options: [
        'c_std=c11',
        ])


am_cflags = [
    '-fstack-protector',
    '-pedantic',
    '-Wstrict-prototypes',
    '-Wundef',
    '-Werror-implicit-function-declaration',
    '-Wformat',
    '-Wformat-security',
    '-Werror=format-security',
    '-Wconversion',
    '-Wno-unused-parameter',
    '-Wunreachable-code',
    '-Wall',
    '-W',
]
add_global_arguments(am_cflags, language: 'c')

gtk = dependency('gtk+-3.0', version: '>=3.22')
glib = dependency('glib-2.0', version: '>=2.50')

conf_data = configuration_data()
openconnect = disabler()
cc = meson.get_compiler('c')
dl = cc.find_library('dl', required: false)

use_openconnect = get_option('use_openconnect')
if(use_openconnect == 'yes')
	openconnect = dependency('openconnect', version: '>=5.99')
endif
if(use_openconnect == 'dynamic' and dl.found())
	openconnect = dependency('openconnect', version: '>=5.99')
	conf_data.set('USE_OPENCONNECT_DYNAMIC', true)
endif
if (use_openconnect == 'check')
	openconnect = dependency('openconnect', version: '>=5.99', required : false)
endif

conf_data.set('USE_OPENCONNECT', openconnect.found())
conf_data.set('USE_STATUS_ICON', get_option('use_status_icon'))
conf_data.set_quoted('GETTEXT_PACKAGE', meson.project_name())
conf_data.set_quoted('CONNMAN_GTK_LOCALEDIR', get_option('localedir'))

conf_data.set('version', meson.project_version())
configure_file(input : 'config.h.in',
               output : 'config.h',
               configuration : conf_data)

extra_includes = [
    include_directories('.'),
]

readmes = [
    'CHANGELOG.md',
    'README.md'
]


install_data(readmes, install_dir : join_paths(get_option('datadir'), meson.project_name(), 'doc'))
subdir('po')
subdir('src')
subdir('data')

