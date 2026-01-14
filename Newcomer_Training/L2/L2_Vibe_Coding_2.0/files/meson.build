project(
    'l2',
    'cpp',
    version: '1.0',
    meson_version: '>=1.1.1',
    default_options: [
        'buildtype=debugoptimized',
        'cpp_std=c++23',
        'warning_level=3',
        'werror=true',
    ],
    license: 'Apache-2.0',
)

add_project_arguments('-Wno-psabi', language: 'cpp')

sdbusplus = dependency('sdbusplus')
phosphor_logging = dependency('phosphor-logging')
boost = dependency('boost')

executable(
    'l2',
    'l2.cpp',
    dependencies: [
        sdbusplus,
        phosphor_logging,
        boost,
    ],
    install: true,
    install_dir: get_option('bindir'),
)
