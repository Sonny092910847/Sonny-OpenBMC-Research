project(
    'sonnychu-l2-service',
    'cpp',
    version: '2.0',
    default_options: [
        'cpp_std=c++23',
        'warning_level=3',
    ],
)

sdbusplus_dep = dependency('sdbusplus')
boost_dep = dependency('boost')

executable(
    'sonnychu-l2-service',
    'sonnychu-l2-service.cpp',
    dependencies: [sdbusplus_dep, boost_dep],
    install: true,
    install_dir: '/usr/bin',
)

install_data(
    'sonnychu-l2-service.service',
    install_dir: '/usr/lib/systemd/system'
)
