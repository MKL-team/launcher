# 基础层：mkl_core（Qt 无关，可独立编译/测试/替换）
add_library(mkl_core STATIC
  Logger.cpp
  PlatformDetector.cpp
  FileUtils.cpp
  StringUtils.cpp
  JsonUtils.cpp
)

target_include_directories(mkl_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(mkl_core PUBLIC cxx_std_17)
