# 单元测试：mkl_core_tests（CTest 接入）
add_executable(mkl_core_tests
  test_main.cpp
  test_logger.cpp
  test_platform_detector.cpp
  test_file_utils.cpp
  test_string_utils.cpp
  test_json_utils.cpp
)
target_link_libraries(mkl_core_tests PRIVATE mkl_core)

add_test(NAME mkl_core_tests COMMAND mkl_core_tests)

# GUI 冒烟测试（需 Qt；无头运行验证主窗口可加载）
add_test(NAME mkl_app_smoke COMMAND mkl_app --smoke-test)
set_tests_properties(mkl_app_smoke PROPERTIES
  ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software"
  TIMEOUT 30
)
