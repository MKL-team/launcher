# cpp/ — C++ 胶水层（R2）

C++ 只做胶水：把 C 核心（mkl_* C ABI）与 Rust 模块（extern "C" FFI）包装成
各原生 UI 壳（Win32/AppKit/GTK3/Kotlin/Swift）可调用的统一桥接接口。

铁律：
- 零业务逻辑：只做类型转换与调用转发
- 统一入口：`mkl_bridge.h`（UI 壳只需包含此头文件）
- 实现里程碑：R2-S4（Windows 壳）

当前：骨架占位，尚无实现。
