# ui/ — 5 套原生 UI 壳（R2）

| 子目录 | 平台 | 技术 | 里程碑 |
| --- | --- | --- | --- |
| windows/ | Windows | Win32 API（自绘，对标 PCL2 风格） | R2-S4 |
| macos/ | macOS | AppKit（Swift） | R2-S5 |
| linux/ | Linux | GTK3（C） | R2-S5 |
| android/ | Android | Kotlin + Jetpack Compose | R2-S6 |
| ios/ | iOS | Swift + UIKit（实验性） | R2-S6 |

壳层职责：渲染与交互、页面结构（6 页面）、平台能力（标题栏/托盘/对话框/Keychain）；
业务逻辑全部在 C/Rust 核心，经 cpp/mkl_bridge.h 调用。

当前：骨架占位，尚无实现。
