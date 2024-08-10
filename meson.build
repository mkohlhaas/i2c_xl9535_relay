project(
  'i2c-c-template',
  'c',
  version: '0.1',
  default_options: ['b_ndebug=if-release', 'warning_level=3', 'werror=false', 'c_std=gnu2x'],
)

source_files = files(
  'i2c.c',
  'i2cdriver.c',
)

incdir = include_directories('error')

executable(
  'i2c-c-template',
  source_files,
  include_directories: incdir,
  install: false,
)
