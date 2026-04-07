add_executable(tst_smoke tst_smoke.cpp)

target_link_libraries(tst_smoke PRIVATE
  Qt6::Core
  Qt6::Test
)

add_test(NAME tst_smoke COMMAND tst_smoke)

# ---------------------------------------------------------------------------
# Logger tests
# ---------------------------------------------------------------------------
add_executable(test_logger test_logger.cpp)

target_link_libraries(test_logger PRIVATE
  MwaCore
  Qt6::Core
  Qt6::Test
)

add_test(NAME test_logger COMMAND test_logger)

# ---------------------------------------------------------------------------
# SettingsManager tests
# ---------------------------------------------------------------------------
add_executable(test_settings_manager test_settings_manager.cpp)

target_link_libraries(test_settings_manager PRIVATE
  MwaCore
  Qt6::Core
  Qt6::Test
)

add_test(NAME test_settings_manager COMMAND test_settings_manager)

# ---------------------------------------------------------------------------
# DeviceInterface tests
# ---------------------------------------------------------------------------
add_executable(test_device_interface test_device_interface.cpp)

target_link_libraries(test_device_interface PRIVATE
  MwaHardware
  Qt6::Core
  Qt6::Test
)

add_test(NAME test_device_interface COMMAND test_device_interface)

# ---------------------------------------------------------------------------
# MainWindow tests
# ---------------------------------------------------------------------------
add_executable(test_main_window test_main_window.cpp)

target_link_libraries(test_main_window PRIVATE
  MwaApp
  MwaCore
  Qt6::Core
  Qt6::Gui
  Qt6::Widgets
  Qt6::Test
)

add_test(NAME test_main_window COMMAND test_main_window)

# ---------------------------------------------------------------------------
# LogTableModel tests
# ---------------------------------------------------------------------------
add_executable(test_log_table_model test_log_table_model.cpp)

target_link_libraries(test_log_table_model PRIVATE
  MwaGui
  MwaCore
  Qt6::Core
  Qt6::Gui
  Qt6::Widgets
  Qt6::Test
)

add_test(NAME test_log_table_model COMMAND test_log_table_model)

# ---------------------------------------------------------------------------
# LogPanel tests
# ---------------------------------------------------------------------------
add_executable(test_log_panel test_log_panel.cpp)

target_link_libraries(test_log_panel PRIVATE
  MwaGui
  MwaCore
  Qt6::Core
  Qt6::Gui
  Qt6::Widgets
  Qt6::Test
)

add_test(NAME test_log_panel COMMAND test_log_panel)

# ---------------------------------------------------------------------------
# DeviceStatusDashboard tests
# ---------------------------------------------------------------------------
add_executable(test_device_status_dashboard
  test_device_status_dashboard.cpp
)

target_link_libraries(test_device_status_dashboard PRIVATE
  MwaGui
  MwaHardware
  MwaCore
  Qt6::Core
  Qt6::Gui
  Qt6::Widgets
  Qt6::Test
)

add_test(NAME test_device_status_dashboard
  COMMAND test_device_status_dashboard
)

# ---------------------------------------------------------------------------
# MockLedController tests
# ---------------------------------------------------------------------------
add_executable(test_mock_led_controller test_mock_led_controller.cpp)

target_link_libraries(test_mock_led_controller PRIVATE
  MwaHardware
  Qt6::Core
  Qt6::Gui
  Qt6::Test
)

add_test(NAME test_mock_led_controller COMMAND test_mock_led_controller)

# ---------------------------------------------------------------------------
# MockPumpController tests
# ---------------------------------------------------------------------------
add_executable(test_mock_pump_controller test_mock_pump_controller.cpp)

target_link_libraries(test_mock_pump_controller PRIVATE
  MwaHardware
  Qt6::Core
  Qt6::Gui
  Qt6::Test
)

add_test(NAME test_mock_pump_controller COMMAND test_mock_pump_controller)

# ---------------------------------------------------------------------------
# MockSignalGeneratorController tests
# ---------------------------------------------------------------------------
add_executable(test_mock_signal_generator_controller
  test_mock_signal_generator_controller.cpp
)

target_link_libraries(test_mock_signal_generator_controller PRIVATE
  MwaHardware
  Qt6::Core
  Qt6::Gui
  Qt6::Test
)

add_test(NAME test_mock_signal_generator_controller
  COMMAND test_mock_signal_generator_controller
)

# ---------------------------------------------------------------------------
# MockNetworkAnalyzerController tests
# ---------------------------------------------------------------------------
add_executable(test_mock_network_analyzer_controller
  test_mock_network_analyzer_controller.cpp
)

target_link_libraries(test_mock_network_analyzer_controller PRIVATE
  MwaHardware
  Qt6::Core
  Qt6::Gui
  Qt6::Test
)

add_test(NAME test_mock_network_analyzer_controller
  COMMAND test_mock_network_analyzer_controller
)

# ---------------------------------------------------------------------------
# MockCameraController tests
# ---------------------------------------------------------------------------
add_executable(test_mock_camera_controller test_mock_camera_controller.cpp)

target_link_libraries(test_mock_camera_controller PRIVATE
  MwaHardware
  Qt6::Core
  Qt6::Gui
  Qt6::Test
)

add_test(NAME test_mock_camera_controller COMMAND test_mock_camera_controller)

# ---------------------------------------------------------------------------
# MockStageController tests
# ---------------------------------------------------------------------------
add_executable(test_mock_stage_controller test_mock_stage_controller.cpp)

target_link_libraries(test_mock_stage_controller PRIVATE
  MwaHardware
  Qt6::Core
  Qt6::Gui
  Qt6::Test
)

add_test(NAME test_mock_stage_controller COMMAND test_mock_stage_controller)

# ---------------------------------------------------------------------------
# LedPanel tests
# ---------------------------------------------------------------------------
add_executable(test_led_panel test_led_panel.cpp)

target_link_libraries(test_led_panel PRIVATE
  MwaPanels
  MwaHardware
  MwaCore
  Qt6::Core
  Qt6::Gui
  Qt6::Widgets
  Qt6::Test
)

add_test(NAME test_led_panel COMMAND test_led_panel)

# ---------------------------------------------------------------------------
# PumpPanel tests
# ---------------------------------------------------------------------------
add_executable(test_pump_panel test_pump_panel.cpp)

target_link_libraries(test_pump_panel PRIVATE
  MwaPanels
  MwaHardware
  MwaCore
  Qt6::Core
  Qt6::Gui
  Qt6::Widgets
  Qt6::Test
)

add_test(NAME test_pump_panel COMMAND test_pump_panel)

# ---------------------------------------------------------------------------
# SignalPanel tests
# ---------------------------------------------------------------------------
add_executable(test_signal_panel test_signal_panel.cpp)

target_link_libraries(test_signal_panel PRIVATE
  MwaPanels
  MwaHardware
  MwaCore
  Qt6::Core
  Qt6::Gui
  Qt6::Widgets
  Qt6::Test
)

add_test(NAME test_signal_panel COMMAND test_signal_panel)

# ---------------------------------------------------------------------------
# CameraPanel tests
# ---------------------------------------------------------------------------
add_executable(test_camera_panel test_camera_panel.cpp)

target_link_libraries(test_camera_panel PRIVATE
  MwaPanels
  MwaHardware
  MwaCore
  Qt6::Core
  Qt6::Gui
  Qt6::Widgets
  Qt6::Test
)

add_test(NAME test_camera_panel COMMAND test_camera_panel)

# ---------------------------------------------------------------------------
# StagePanel tests
# ---------------------------------------------------------------------------
add_executable(test_stage_panel test_stage_panel.cpp)

target_link_libraries(test_stage_panel PRIVATE
  MwaPanels
  MwaHardware
  MwaCore
  Qt6::Core
  Qt6::Gui
  Qt6::Widgets
  Qt6::Test
)

add_test(NAME test_stage_panel COMMAND test_stage_panel)

# ---------------------------------------------------------------------------
# CommandQueue tests
# ---------------------------------------------------------------------------
add_executable(test_command_queue test_command_queue.cpp)

target_link_libraries(test_command_queue PRIVATE
  MwaHardware
  Qt6::Core
  Qt6::Test
)

add_test(NAME test_command_queue COMMAND test_command_queue)

# ---------------------------------------------------------------------------
# DeviceManager tests
# ---------------------------------------------------------------------------
add_executable(test_device_manager test_device_manager.cpp)

target_link_libraries(test_device_manager PRIVATE
  MwaHardware
  MwaCore
  Qt6::Core
  Qt6::Test
)

add_test(NAME test_device_manager COMMAND test_device_manager)

# ---------------------------------------------------------------------------
# ErrorHandler tests
# ---------------------------------------------------------------------------
add_executable(test_error_handler test_error_handler.cpp)

target_link_libraries(test_error_handler PRIVATE
  MwaHardware
  MwaCore
  Qt6::Core
  Qt6::Test
)

add_test(NAME test_error_handler COMMAND test_error_handler)

# ---------------------------------------------------------------------------
# ExperimentSession tests
# ---------------------------------------------------------------------------
add_executable(test_experiment_session test_experiment_session.cpp)

target_link_libraries(test_experiment_session PRIVATE
  MwaAnalysis
  Qt6::Core
  Qt6::Gui
  Qt6::Test
)

add_test(NAME test_experiment_session COMMAND test_experiment_session)

# ---------------------------------------------------------------------------
# SessionSerializer tests
# ---------------------------------------------------------------------------
add_executable(test_session_serializer test_session_serializer.cpp)

target_link_libraries(test_session_serializer PRIVATE
  MwaAnalysis
  Qt6::Core
  Qt6::Gui
  Qt6::Test
)

add_test(NAME test_session_serializer COMMAND test_session_serializer)

# ---------------------------------------------------------------------------
# VnaCsvExporter tests
# ---------------------------------------------------------------------------
add_executable(test_vna_csv_exporter test_vna_csv_exporter.cpp)

target_link_libraries(test_vna_csv_exporter PRIVATE
  MwaAnalysis
  Qt6::Core
  Qt6::Test
)

add_test(NAME test_vna_csv_exporter COMMAND test_vna_csv_exporter)

# ---------------------------------------------------------------------------
# CameraFrameExporter tests
# ---------------------------------------------------------------------------
add_executable(test_camera_frame_exporter test_camera_frame_exporter.cpp)

target_link_libraries(test_camera_frame_exporter PRIVATE
  MwaAnalysis
  Qt6::Core
  Qt6::Gui
  Qt6::Test
)

add_test(NAME test_camera_frame_exporter COMMAND test_camera_frame_exporter)

# ---------------------------------------------------------------------------
# AnalysisPanel tests
# ---------------------------------------------------------------------------
add_executable(test_analysis_panel test_analysis_panel.cpp)

target_link_libraries(test_analysis_panel PRIVATE
  MwaAnalysis
  MwaPanels
  Qt6::Core
  Qt6::Gui
  Qt6::Test
  Qt6::Widgets
)

add_test(NAME test_analysis_panel COMMAND test_analysis_panel)

# ---------------------------------------------------------------------------
# SerialUtils tests (require Qt6::SerialPort)
# ---------------------------------------------------------------------------
if(TARGET Qt6::SerialPort)
  add_executable(test_serial_utils test_serial_utils.cpp)

  target_link_libraries(test_serial_utils PRIVATE
    MwaHardware
    Qt6::Core
    Qt6::Test
  )

  add_test(NAME test_serial_utils COMMAND test_serial_utils)

  # -------------------------------------------------------------------------
  # LedController tests (real driver, no hardware needed for state tests)
  # -------------------------------------------------------------------------
  add_executable(test_led_controller test_led_controller.cpp)

  target_link_libraries(test_led_controller PRIVATE
    MwaHardware
    Qt6::Core
    Qt6::Test
  )

  add_test(NAME test_led_controller COMMAND test_led_controller)

  # -------------------------------------------------------------------------
  # StageController tests (real driver, no hardware needed for state tests)
  # -------------------------------------------------------------------------
  add_executable(test_stage_controller test_stage_controller.cpp)

  target_link_libraries(test_stage_controller PRIVATE
    MwaHardware
    Qt6::Core
    Qt6::Test
  )

  add_test(NAME test_stage_controller COMMAND test_stage_controller)
endif()
