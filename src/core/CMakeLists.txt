add_library(MwaCore STATIC
  logger.h
  logger.cpp
  settings_manager.h
  settings_manager.cpp
)

target_include_directories(MwaCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(MwaCore PUBLIC Qt6::Core)
