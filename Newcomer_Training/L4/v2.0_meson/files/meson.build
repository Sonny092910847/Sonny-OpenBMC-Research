project(
    'gpio-monitor',
    'cpp',
    version: '2.0',
    default_options: [
        'cpp_std=c++23',
        'warning_level=3',
    ],
)

# Find libgpiod C++ dependency
gpiodcxx = dependency('libgpiodcxx', required: true)

# Build the executable
executable(
    'gpio-monitor',
    'gpio-monitor.cpp',
    dependencies: [gpiodcxx],
    install: true,
    install_dir: '/usr/bin',
)

# Install systemd service file
install_data(
    'gpio-monitor.service',
    install_dir: '/usr/lib/systemd/system'
)
