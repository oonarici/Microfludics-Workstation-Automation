cmake_minimum_required(VERSION 3.20)
project(DeviceControlApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

#---- Qt ----
find_package(Qt6 REQUIRED COMPONENTS Widgets)

#---- Core library ----

add_library(core
src/core_dummy.cpp
)
target_include_directories(core PUBLIC include)

#---- Application ----

add_executable(app
app/main.cpp
)
target_link_libraries(app PRIVATE core Qt6::Widgets)

#---- Tests ----

add_executable(test_smoke
tests/test_smoke.cpp
)
target_link_libraries(test_smoke PRIVATE core)

add_test(NAME smoke_test COMMAND test_smoke)
