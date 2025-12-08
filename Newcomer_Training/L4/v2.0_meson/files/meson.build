project(
    'gpio-monitor',
    'cpp',
    version: '2.0',
    default_options: [
        'cpp_std=c++23',
        'warning_level=3',
    ],
)

gpiodcxx = dependency('libgpiodcxx')

executable(
    'gpio-monitor',
    'gpio-monitor.cpp',
    dependencies: [gpiodcxx],
    install: true,
    install_dir: '/usr/bin',
)

install_data(
    'gpio-monitor.service',
    install_dir: '/usr/lib/systemd/system'
)
