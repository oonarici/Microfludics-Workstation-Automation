add_executable(MWA main.cpp)

target_link_libraries(MWA PRIVATE
  Qt6::Core
  Qt6::Gui
  Qt6::Widgets
)
