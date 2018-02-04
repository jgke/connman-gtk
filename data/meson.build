install_data('net.connman.gtk.gschema.xml',install_dir: join_paths(get_option('datadir'), 'glib-2.0', 'schemas'))
install_man('connman-gtk.1')


i18n = import('i18n')
desktop_file = i18n.merge_file(
  input: 'connman-gtk.desktop.in',
  output: 'connman-gtk.desktop',
  type: 'desktop',
  po_dir: '../po',
  install: true,
  install_dir: join_paths(get_option('datadir'), 'applications'),
)
               
