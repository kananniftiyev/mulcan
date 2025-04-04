cmake_minimum_required (VERSION 3.30)
project(mulcan VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB_RECURSE HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp)

include(FetchContent)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(spdlog REQUIRED)

if(UNIX)
  FetchContent_Declare(
    vk-bootstrap
    GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap.git
    GIT_TAG        main
  )

  FetchContent_Declare(
    VulkanMemoryAllocator
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG        master
  )

  FetchContent_MakeAvailable(vk-bootstrap VulkanMemoryAllocator)
endif(UNIX)

if(WIN32)
  find_package(VulkanMemoryAllocator CONFIG REQUIRED)
  find_package(vk-bootstrap CONFIG REQUIRED)
endif(WIN32)

add_library (mulcan STATIC ${SRC_FILES} ${HEADER_FILES})

target_include_directories(mulcan PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  )

if (WIN32)
  target_link_libraries(mulcan PRIVATE
    Vulkan::Vulkan
    glfw
    glm::glm
    GPUOpen::VulkanMemoryAllocator
    spdlog::spdlog
    vk-bootstrap::vk-bootstrap 
    vk-bootstrap::vk-bootstrap-compiler-warnings
  )
endif(WIN32)

if(UNIX)
  target_include_directories(mulcan PRIVATE
    ${vma_SOURCE_DIR}/include  )
  target_link_libraries(mulcan PRIVATE
    Vulkan
    glfw
    glm
    GPUOpen::VulkanMemoryAllocator
    spdlog
    vk-bootstrap::vk-bootstrap
  )
endif(UNIX)

