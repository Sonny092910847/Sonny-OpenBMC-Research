project(
    'bmc-l1-hello',
    'cpp',
    version: '2.0',
)

executable(
    'bmc-l1-hello',
    'bmc_L1_ID835051.cpp',
    install: true,
    install_dir: '/usr/bin',
)

install_data(
    'bmc.L1.id835051.service',
    install_dir: '/usr/lib/systemd/system'
)
