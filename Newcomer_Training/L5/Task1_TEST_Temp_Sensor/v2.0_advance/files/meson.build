project(
    'test-temp-sensor',
    'cpp',
    version: '1.0',
    default_options: [
        'cpp_std=c++23',
        'warning_level=3',
    ],
)

sdbusplus_dep = dependency('sdbusplus')
boost_dep = dependency('boost')

executable(
    'test-temp-sensor',
    'test-temp-sensor.cpp',
    dependencies: [sdbusplus_dep, boost_dep],
    install: true,
    install_dir: '/usr/bin',
)

install_data(
    'test-temp-sensor.service',
    install_dir: '/usr/lib/systemd/system'
)
