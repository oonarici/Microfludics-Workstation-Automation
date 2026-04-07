add_library(MwaPanels STATIC
  panel_colors.h
  led_panel.h
  led_panel.cpp
  pump_panel.h
  pump_panel.cpp
  signal_panel.h
  signal_panel.cpp
  camera_panel.h
  camera_panel.cpp
  stage_panel.h
  stage_panel.cpp
  analysis_panel.h
  analysis_panel.cpp
)

target_include_directories(MwaPanels PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..)

target_link_libraries(MwaPanels PUBLIC
  MwaAnalysis
  MwaCore
  MwaHardware
  Qt6::Concurrent
  Qt6::Core
  Qt6::Gui
  Qt6::Widgets
)
