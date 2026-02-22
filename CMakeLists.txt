cmake_minimum_required(VERSION 3.24)

project(MicrofluidicsAutomationTool VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(MICROFLUIDICS_ENABLE_QT "Enable Qt integration" ON)

add_library(microfluidics_core
    src/app.cpp
)

target_include_directories(microfluidics_core
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_features(microfluidics_core PUBLIC cxx_std_20)

if(MICROFLUIDICS_ENABLE_QT)
    find_package(Qt6 COMPONENTS Core QUIET)
    if(Qt6_FOUND)
        target_link_libraries(microfluidics_core PUBLIC Qt6::Core)
        target_compile_definitions(microfluidics_core PUBLIC MICROFLUIDICS_HAVE_QT=1)
    else()
        message(STATUS "Qt6 not found; building without Qt integration")
    endif()
endif()

add_executable(microfluidics_app
    src/main.cpp
)

target_link_libraries(microfluidics_app PRIVATE microfluidics_core)

enable_testing()
add_executable(smoke_test tests/smoke_test.cpp)
target_link_libraries(smoke_test PRIVATE microfluidics_core)
add_test(NAME smoke_test COMMAND smoke_test)
