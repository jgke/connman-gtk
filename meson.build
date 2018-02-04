project('connman-gtk', 'c', version : '1.1.1')

gtkdep = dependency('gtk+-3.0', version: '>=3.22')
glibdep = dependency('glib-2.0', version: '>=2.50')

conf_data = configuration_data()
openconnect = disabler()

use_openconnect = get_option('use_openconnect')
if(use_openconnect == 'yes')
	openconnect = dependency('openconnect', version: '>=5.99')
endif
if(use_openconnect == 'dynamic')
	openconnect = dependency('openconnect', version: '>=5.99')
	conf_data.set('USE_OPENCONNECT_DYNAMIC', true)
endif
if (use_openconnect == 'check')
	openconnect = dependency('openconnect', version: '>=5.99', required : false)
endif

conf_data.set('USE_OPENCONNECT', openconnect.found())
conf_data.set('USE_STATUS_ICON', get_option('use_status_icon'))

conf_data.set('version', meson.project_version())
configure_file(input : 'project_config.h.in',
               output : 'project_config.h',
               configuration : conf_data)


subdir('po')
subdir('lib')
subdir('src')

