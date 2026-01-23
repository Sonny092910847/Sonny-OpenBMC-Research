project(
    'l4-gpio-monitor',
    'cpp',
    version: '1.0',
    default_options: [
        'cpp_std=c++23',
        'warning_level=3',
        'werror=true',
    ],
)

# Dependencies
sdbusplus = dependency('sdbusplus')
phosphor_logging = dependency('phosphor-logging')
boost = dependency('boost')
libgpiod = dependency('libgpiod')

# Boost compile arguments
boost_args = [
    '-DBOOST_ASIO_DISABLE_THREADS',
    '-DBOOST_ERROR_CODE_HEADER_ONLY',
    '-DBOOST_SYSTEM_NO_DEPRECATED',
]

# Build executable
executable(
    'l4-gpio-monitor',
    'l4.cpp',
    dependencies: [
        sdbusplus,
        phosphor_logging,
        boost,
        libgpiod,
    ],
    cpp_args: boost_args,
    install: true,
    install_dir: get_option('bindir'),
)
