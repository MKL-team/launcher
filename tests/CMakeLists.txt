# 单元测试：mkl_core_tests（CTest 接入）
add_executable(mkl_core_tests
  test_main.c
  test_logger.c
  test_platform_detector.c
  test_file_utils.c
  test_string_utils.c
  test_json.c
)
target_link_libraries(mkl_core_tests PRIVATE mkl_core)

add_test(NAME mkl_core_tests COMMAND mkl_core_tests)

# CLI 冒烟测试（无头自检：日志/平台/JSON/SHA-1 全链路）
add_test(NAME mkl_app_smoke COMMAND mkl_app --smoke-test)
set_tests_properties(mkl_app_smoke PROPERTIES TIMEOUT 30)
