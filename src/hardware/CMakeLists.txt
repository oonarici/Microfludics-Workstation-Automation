add_library(MwaHardware STATIC
  device_interface.h
  device_interface.cpp
)

target_include_directories(MwaHardware PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(MwaHardware PUBLIC Qt6::Core)
