add_library(MwaAnalysis STATIC
  experiment_session.h
  experiment_session.cpp
  session_serializer.h
  session_serializer.cpp
)

target_include_directories(MwaAnalysis PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(MwaAnalysis PUBLIC Qt6::Core Qt6::Gui)
