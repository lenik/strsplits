project('strsplit', 'c',
  version: '1.0.0',
  license: 'GPL-3.0-or-later',
  default_options: [
    'c_std=c99',
    'warning_level=2',
  ],
)

# Configuration
conf_data = configuration_data()
conf_data.set_quoted('VERSION', meson.project_version())

# 2. Create the header file (this fixes the error)
configure_file(
  output: 'config.h',
  configuration: conf_data,
)

# Main executable
executable('strsplit',
  'strsplit.c',
  install: true,
  install_dir: get_option('prefix') / 'bin',
)

# Man page
install_data('strsplit.1',
  install_dir: get_option('prefix') / 'share' / 'man' / 'man1',
)

# Bash completion
install_data('bash-completion/strsplit',
  install_dir: get_option('prefix') / 'share' / 'bash-completion' / 'completions',
)

# Summary
summary({
  'Version': meson.project_version(),
  'Prefix': get_option('prefix'),
  'Install prefix': get_option('prefix'),
}, bool_yn: true)
