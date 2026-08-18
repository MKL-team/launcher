# 基础层：mkl_core（C11，可独立编译/测试/替换）
find_package(Threads REQUIRED)

add_library(mkl_core STATIC
  logger.c
  platform_detector.c
  file_utils.c
  string_utils.c
  json.c
)

target_include_directories(mkl_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mkl_core PUBLIC Threads::Threads)
