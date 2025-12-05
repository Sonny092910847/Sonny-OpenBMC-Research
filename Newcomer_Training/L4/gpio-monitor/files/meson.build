project(
    'gpio-monitor',
    'cpp',
    version: '1.0',
    default_options: [
        'cpp_std=c++23',
        'warning_level=3',
    ],
)

# Find libgpiod dependency
gpiodcxx = dependency('libgpiodcxx', required: true)

# Build the executable
executable(
    'gpio-monitor',
    'gpio-monitor.cpp',
    dependencies: [gpiodcxx],
    install: true,
    install_dir: '/usr/bin',
)
