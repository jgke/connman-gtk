connman_gtk_sources = [
'agent.c',
'settings.c',
'technology.c',
'main.c',
'settings_content.c',
'configurator.c',
'util.c',
'connection.c',
'openconnect_helper.c',
'status.c',
'vpn.c',
'dialog.c',
'service.c',
'style.c',
'wireless.c',
]

if (not openconnect.found())
	openconnect = declare_dependency()
endif

executable(meson.project_name(),
	   connman_gtk_sources,
	   dependencies : [gtk, glib, openconnect, dl],
	   include_directories: extra_includes,
	   install: true)
