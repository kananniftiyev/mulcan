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


find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(spdlog CONFIG REQUIRED)

FetchContent_MakeAvailable(mulcan)

add_executable(main main.cpp)

target_link_libraries(main PUBLIC mulcan Vulkan
  glfw
  glm)
target_include_directories(mulcan PUBLIC 
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)


