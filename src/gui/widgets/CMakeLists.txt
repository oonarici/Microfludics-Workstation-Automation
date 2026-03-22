add_library(MwaGui STATIC
  log_table_model.h
  log_table_model.cpp
  log_panel.h
  log_panel.cpp
  device_status_dashboard.h
  device_status_dashboard.cpp
)

target_include_directories(MwaGui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..)

target_link_libraries(MwaGui PUBLIC
  MwaCore
  MwaHardware
  Qt6::Core
  Qt6::Gui
  Qt6::Widgets
)
