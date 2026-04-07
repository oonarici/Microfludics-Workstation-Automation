add_library(MwaDialogs STATIC
  new_session_dialog.h
  new_session_dialog.cpp
)

target_include_directories(MwaDialogs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..)

target_link_libraries(MwaDialogs PUBLIC
  Qt6::Core
  Qt6::Gui
  Qt6::Widgets
)
