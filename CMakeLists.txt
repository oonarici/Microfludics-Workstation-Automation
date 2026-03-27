cmake_minimum_required(VERSION 3.21)
project(MWA VERSION 0.1.0 LANGUAGES CXX)

# Enforce C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt tooling
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Test)
find_package(Qt6 OPTIONAL_COMPONENTS SerialPort)

# Testing
include(CTest)
enable_testing()

# Compiler warning flags
add_compile_options(
  $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Wall>
  $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Wextra>
  $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Wpedantic>
  $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Werror>
  $<$<CXX_COMPILER_ID:MSVC>:/W4>
  $<$<CXX_COMPILER_ID:MSVC>:/WX>
)

# Subdirectories
add_subdirectory(src)
add_subdirectory(tests)
