project('ffxapiProxy', 
    'cpp',
    default_options: [
        'cpp_std=c++20',
        'warning_level=2'
    ])

cpp = meson.get_compiler('cpp')

lib_dxgi = cpp.find_library('dxgi')

incdir = include_directories('include')

link_args = [
    '-static',
    '-static-libgcc',
    '-static-libstdc++',
    '-Wl,--file-alignment=4096'
]

compiler_args = ['-Wno-unused-parameter']

add_project_arguments(cpp.get_supported_arguments(compiler_args), language: 'cpp')
add_project_link_arguments(cpp.get_supported_link_arguments(link_args), language: 'cpp')

subdir('src')
