add_subdirectory(core)
add_subdirectory(hardware)
add_subdirectory(gui)
add_subdirectory(app)

add_executable(MWA main.cpp)

target_link_libraries(MWA PRIVATE
  MwaApp
  MwaCore
  MwaHardware
  Qt6::Core
  Qt6::Gui
  Qt6::Widgets
)
