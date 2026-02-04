project(
    'sensor-monitor',
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

# Compiler arguments for single-threaded boost::asio
boost_args = [
    '-DBOOST_ASIO_DISABLE_THREADS',
    '-DBOOST_ERROR_CODE_HEADER_ONLY',
    '-DBOOST_SYSTEM_NO_DEPRECATED',
]

# Build executable
executable(
    'l2-sensor-monitor',
    'sensor_monitor.cpp',
    dependencies: [
        sdbusplus,
        phosphor_logging,
        boost,
    ],
    cpp_args: boost_args,
    install: true,
    install_dir: get_option('bindir'),
)

# Install systemd service file
systemd = dependency('systemd')
systemd_system_unit_dir = systemd.get_variable(
    'systemdsystemunitdir',
    pkgconfig_define: ['prefix', get_option('prefix')]
)

install_data(
    'xyz.openbmc_project.SensorMonitor.service',
    install_dir: systemd_system_unit_dir,
)
