add_library(MwaApp STATIC
  main_window.h
  main_window.cpp
)

target_include_directories(MwaApp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(MwaApp PUBLIC
  MwaCore
  MwaGui
  Qt6::Core
  Qt6::Gui
  Qt6::Widgets
)
