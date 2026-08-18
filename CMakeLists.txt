cmake_minimum_required(VERSION 3.21)
project(MKL VERSION 0.0.2 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# 统一产物输出目录（build/bin），保证 CI 路径确定
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
foreach(cfg RELEASE DEBUG RELWITHDEBINFO MINSIZEREL)
  string(TOUPPER ${cfg} CFG)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CFG} ${CMAKE_BINARY_DIR}/bin)
endforeach()

# MSVC 统一按 UTF-8 解析源码（源文件含中文日志/注释）
if(MSVC)
  add_compile_options(/utf-8)
endif()

add_subdirectory(src/core)
add_subdirectory(src/entry)

enable_testing()
add_subdirectory(tests)
