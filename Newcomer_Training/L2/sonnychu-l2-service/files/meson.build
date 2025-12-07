project(
    'sonnychu-l2-service',
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
    'sonnychu-l2-service',   //產生的執行檔名稱
    'sonnychu-l2-service.cpp',
    dependencies: [sdbusplus_dep, boost_dep], //需要的函式庫
    install: true,
    install_dir: '/usr/bin', //編譯完後安裝到的路徑
)
