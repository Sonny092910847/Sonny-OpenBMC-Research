project(
    'training-sensor-monitor',
    'cpp',
    version: '1.0.0',
    license: 'Apache-2.0',
    default_options: [
        'cpp_std=c++23',
        'warning_level=3',
        'werror=true',
    ],
    meson_version: '>=1.1.1',
)

# Compiler arguments
cpp = meson.get_compiler('cpp')

# Dependencies
sdbusplus_dep = dependency('sdbusplus')
boost_dep = dependency('boost')

# Source files
sources = files(
    'src/main.cpp',
    'src/sensor_monitor.cpp',
)

# Build executable
executable(
    'training-sensor-monitor',
    sources,
    dependencies: [
        sdbusplus_dep,
        boost_dep,
    ],
    install: true,
    install_dir: get_option('bindir'),
)

# Install systemd service file
systemd = dependency('systemd', required: false)
if systemd.found()
    systemd_system_unit_dir = systemd.get_variable(
        pkgconfig: 'systemdsystemunitdir',
    )
    configure_file(
        input: 'training-sensor-monitor.service',
        output: 'training-sensor-monitor.service',
        copy: true,
        install: true,
        install_dir: systemd_system_unit_dir,
    )
endif
