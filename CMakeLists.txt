cmake_minimum_required(VERSION 3.21)
project(MKL VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

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
