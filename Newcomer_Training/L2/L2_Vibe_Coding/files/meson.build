project('l2', 'cpp',
    version: '1.0',
    default_options: [
        'cpp_std=c++20',
        'warning_level=3',
    ])

systemd_dep = dependency('libsystemd')
phosphor_logging_dep = dependency('phosphor-logging')

executable('l2',
    'l2.cpp',
    dependencies: [
        systemd_dep,
        phosphor_logging_dep,
    ],
    install: true,
    install_dir: get_option('bindir'))
