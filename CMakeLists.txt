cmake_minimum_required(VERSION 3.21)
project(MKL VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 统一产物输出目录（build/bin），保证 CI 路径确定
# 多配置生成器（VS）默认追加配置子目录，这里显式去掉，保证各平台路径一致
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
foreach(cfg RELEASE DEBUG RELWITHDEBINFO MINSIZEREL)
  string(TOUPPER ${cfg} CFG)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CFG} ${CMAKE_BINARY_DIR}/bin)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CFG} ${CMAKE_BINARY_DIR}/lib)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CFG} ${CMAKE_BINARY_DIR}/lib)
endforeach()

# MSVC 统一按 UTF-8 解析源码（源文件含中文日志/注释）
if(MSVC)
  add_compile_options(/utf-8)
endif()

option(MKL_BUILD_TESTS "Build unit tests" ON)

add_subdirectory(src/core)
add_subdirectory(src/entry)

if(MKL_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
