# platform/ — 平台抽象垫片（R2）

C 实现的平台适配器 + 少量汇编（platform/asm/）：

| 适配器 | 职责 | 状态 |
| --- | --- | --- |
| process.h/.c | 进程创建/监控/终止/输出捕获 | R2-S3 |
| credential.h/.c | 凭证加密存取（DPAPI/Keychain/libsecret/Keystore） | R2-S2 |
| platform_ui.h/.c | 系统级 UI 回调（FATAL 弹窗等） | R2-S2 |
| asm/ | SHA-1/SHA-256 SIMD 加速（含 C fallback） | 可选优化 |

铁律：汇编必须提供 C 函数签名 + 纯 C fallback，运行时按平台特性选择。

当前：骨架占位，尚无实现。
