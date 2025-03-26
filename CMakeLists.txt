cmake_minimum_required (VERSION 3.30)
project(mulcan VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB_RECURSE HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(spdlog CONFIG REQUIRED)

include(FetchContent)

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


FetchContent_MakeAvailable(VMA VkBootstrap)

add_library(VMA_STATIC STATIC IMPORTED)
set_target_properties(VMA_STATIC PROPERTIES
  IMPORTED_LOCATION "${VMA_BINARY_DIR}/VMA" # Adjust based on your build output
  INTERFACE_INCLUDE_DIRECTORIES "${VMA_SOURCE_DIR}/include"
)

add_library (mulcan STATIC ${SRC_FILES} ${HEADER_FILES})

target_include_directories(mulcan PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${VMA_SOURCE_DIR}/include 
  ${VkBootstrap_SOURCE_DIR}/include
  )

target_link_libraries(mulcan PUBLIC
  Vulkan
  glfw
  glm
  VMA
  vk-bootstrap
  VMA_STATIC
  spdlog::spdlog
)