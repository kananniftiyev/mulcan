project('mulcanProject', 'cpp', version: '0.1.0')

cxx_std = 'c++20'

vulkan_dep = dependency('vulkan', required: true)
glfw_dep = dependency('glfw3', required: true)
glm_dep = dependency('glm', required: true)
spdlog_dep = dependency('spdlog', required: true)

message(meson.current_source_dir())

vma_dep = declare_dependency(include_directories: include_directories('./submodules/vma/include'))

vma_src = files('./submodules/vma/src/VmaUsage.cpp')

vk_bootsrap_dep = declare_dependency(include_directories: include_directories('./submodules/vk-bootstrap/src'))
vk_bootsrap_src = files('./submodules/vk-bootstrap/src/VkBootstrap.cpp')

all_deps = [vulkan_dep, glfw_dep, glm_dep, spdlog_dep, vma_dep, vk_bootsrap_dep]

mulcan_src = files('src/mulcan.cpp', 'src/mulcan_infos.cpp')

# Mulcan lib
mulcan = static_library(
    'mulcan',
    mulcan_src + vma_src + vk_bootsrap_src,
    dependencies: all_deps,
    include_directories: include_directories('include'),
)

mulcan_dep = declare_dependency(
    link_with: mulcan,
    dependencies: all_deps,
    include_directories: include_directories('include'),
)

# Mulcan test exe
executable('mulcanTester', 'tests/main.cpp', dependencies: [mulcan_dep])