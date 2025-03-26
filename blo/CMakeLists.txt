cmake_minimum_required (VERSION 3.30)
project(main VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
  mulcan
  GIT_REPOSITORY https://github.com/kananniftiyev/mulcan.git
  GIT_TAG        main
)

FetchContent_Declare(
  VkBootstrap
  GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap.git
  GIT_TAG        main
)

FetchContent_Declare(
  VMA
  GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
  GIT_TAG        master
)


find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(spdlog CONFIG REQUIRED)

FetchContent_MakeAvailable(mulcan VMA VkBootstrap)

add_library(VMA_STATIC STATIC IMPORTED)
set_target_properties(VMA_STATIC PROPERTIES
  IMPORTED_LOCATION "${VMA_BINARY_DIR}/VMA" # Adjust based on your build output
  INTERFACE_INCLUDE_DIRECTORIES "${VMA_SOURCE_DIR}/include"
)

add_executable(main main.cpp)

target_link_libraries(main PUBLIC mulcan Vulkan
  glfw
  glm
 vk-bootstrap
  VMA_STATIC)
target_include_directories(mulcan PUBLIC 
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)


