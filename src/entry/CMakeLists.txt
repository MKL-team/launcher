# 入口层：mkl_app（C 命令行工具，R2 骨架）
# UI 壳（各平台原生）里程碑后，本程序为 CLI/自检入口
add_executable(mkl_app main.c)
target_link_libraries(mkl_app PRIVATE mkl_core)
