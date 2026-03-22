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
