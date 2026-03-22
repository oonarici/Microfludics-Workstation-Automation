add_library(MwaHardware INTERFACE)

target_include_directories(MwaHardware INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(MwaHardware INTERFACE Qt6::Core)

target_sources(MwaHardware INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/device_interface.h
)
