add_subdirectory(core)
add_subdirectory(hardware)

add_executable(MWA main.cpp)

target_link_libraries(MWA PRIVATE
  MwaCore
  MwaHardware
  Qt6::Core
  Qt6::Gui
  Qt6::Widgets
)
