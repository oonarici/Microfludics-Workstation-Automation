add_library(MwaHardware STATIC
  device_interface.h
  device_interface.cpp
  command_queue.h
  command_queue.cpp
  device_manager.h
  device_manager.cpp
  error_handler.h
  error_handler.cpp

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

  # SCPI
  scpi/scpi_transport.h
  scpi/scpi_client.h
  scpi/scpi_client.cpp
)

target_include_directories(MwaHardware PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(MwaHardware PUBLIC Qt6::Core Qt6::Gui MwaCore)

# Real hardware drivers — require Qt6::SerialPort
if(TARGET Qt6::SerialPort)
  target_sources(MwaHardware PRIVATE
    serial_utils.h
    serial_utils.cpp
    led/led_controller.h
    led/led_controller.cpp
    stage/stage_controller.h
    stage/stage_controller.cpp
    scpi/serial_transport.h
    scpi/serial_transport.cpp
  )
  target_link_libraries(MwaHardware PUBLIC Qt6::SerialPort)
  target_compile_definitions(MwaHardware PUBLIC MWA_HAS_SERIAL_PORT)
endif()

# Cetoni Nemesys pump driver — requires QmixSDK (Windows/Linux only)
if(QmixSDK_FOUND)
  target_sources(MwaHardware PRIVATE
    pump/pump_controller.h
    pump/pump_controller.cpp
  )
  target_link_libraries(MwaHardware PRIVATE QmixSDK::QmixSDK)
  target_compile_definitions(MwaHardware PRIVATE MWA_HAS_QMIXSDK)
endif()
