cmake_minimum_required(VERSION 3.13)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Enable/Disable export of compile commands")

set(CMAKE_TOOLCHAIN_FILE /Users/school/vcpkg/scripts/buildsystems/vcpkg.cmake CACHE PATH "vcpkg toolchain file")

project(shell-starter-cpp)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.hpp)

set(CMAKE_CXX_STANDARD 23) # Enable the C++23 standard

add_executable(shell ${SOURCE_FILES})

find_package(PkgConfig REQUIRED)
pkg_check_modules(readline REQUIRED IMPORTED_TARGET readline)
target_link_libraries(shell PRIVATE PkgConfig::readline)