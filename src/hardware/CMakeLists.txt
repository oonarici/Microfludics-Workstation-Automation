add_library(MwaHardware STATIC
  device_interface.h
  device_interface.cpp
  command_queue.h
  command_queue.cpp

  # LED
  led/led_controller_interface.h
  led/mock_led_controller.h
  led/mock_led_controller.cpp

  # Pump
  pump/pump_controller_interface.h
  pump/mock_pump_controller.h
  pump/mock_pump_controller.cpp

  # Signal Generator
  signal_generator/signal_generator_controller_interface.h
  signal_generator/mock_signal_generator_controller.h
  signal_generator/mock_signal_generator_controller.cpp

  # Network Analyzer
  network_analyzer/network_analyzer_controller_interface.h
  network_analyzer/mock_network_analyzer_controller.h
  network_analyzer/mock_network_analyzer_controller.cpp

  # Camera
  camera/camera_controller_interface.h
  camera/mock_camera_controller.h
  camera/mock_camera_controller.cpp

  # Stage
  stage/stage_controller_interface.h
  stage/mock_stage_controller.h
  stage/mock_stage_controller.cpp
)

target_include_directories(MwaHardware PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(MwaHardware PUBLIC Qt6::Core Qt6::Gui)
