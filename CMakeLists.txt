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
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(vk-bootstrap CONFIG REQUIRED)

include(FetchContent)

add_library (mulcan STATIC ${SRC_FILES} ${HEADER_FILES})

target_include_directories(mulcan PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  )

target_link_libraries(mulcan PRIVATE
  Vulkan
  glfw
  glm
  VMA
  vk-bootstrap
  GPUOpen::VulkanMemoryAllocator
  spdlog::spdlog
  vk-bootstrap::vk-bootstrap 
  vk-bootstrap::vk-bootstrap-compiler-warnings
)