# 入口层：mkl_app（Qt Quick 空白窗口应用）
find_package(Qt6 REQUIRED COMPONENTS Quick Qml)

add_executable(mkl_app
  main.cpp
  resources.qrc
)

target_link_libraries(mkl_app PRIVATE Qt6::Quick Qt6::Qml mkl_core)

set_target_properties(mkl_app PROPERTIES
  MACOSX_BUNDLE TRUE
  MACOSX_BUNDLE_BUNDLE_NAME "MKL"
  MACOSX_BUNDLE_GUI_IDENTIFIER "org.mkl-team.launcher"
  MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
  WIN32_EXECUTABLE TRUE
)
