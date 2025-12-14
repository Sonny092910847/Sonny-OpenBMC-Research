project(
    'test-temp-oem',
    'cpp',
    version: '1.0',
    default_options: [
        'cpp_std=c++23',
        'warning_level=3',
        'b_lundef=false',
    ],
)


ipmid_dep = dependency('libipmid')
phosphor_logging_dep = dependency('phosphor-logging')
openssl_dep = dependency('openssl')


shared_library(
    'test-temp-oem',
    'test-temp-oem.cpp',
    dependencies: [ipmid_dep, phosphor_logging_dep, openssl_dep],
    install: true,
    install_dir: '/usr/lib/ipmid-providers',
)
