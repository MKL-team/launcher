# 麦块启动器（MKL）开发规格书（R2 架构修正版）

> **版本**：0.0.2-alpha（架构基线 R2）
> **性质**：在 v0.0.1 规格基础上**推倒技术栈重来**的架构修正版。功能需求、模块划分、平台要求不变；语言构成与 UI 方案按用户决策重写。
> **生效决策（2025-08 确认）**：
> 1. **核心引擎：主要 C**（基础/数据/服务层）
> 2. **中量 Rust**（网络/认证/自更新等需要内存安全与生态的模块）
> 3. **少量汇编**（SHA 族 SIMD 等平台热路径，可选优化）
> 4. **C++ 只做胶水**（UI 壳与核心之间的绑定层，不承载业务逻辑）
> 5. **UI 外包平台原生**：Windows / macOS / Linux / Android / iOS **各一套原生 UI 壳**

---

## 目录

- [第一部分：技术架构总则（R2 变更）](#第一部分技术架构总则r2-变更)
- [第二部分：语言构成与边界](#第二部分语言构成与边界)
- [第三部分：31 模块语言映射](#第三部分31-模块语言映射)
- [第四部分：5 套原生 UI 壳规范](#第四部分5-套原生-ui-壳规范)
- [第五部分：错误处理与命名规范（R2）](#第五部分错误处理与命名规范r2)
- [第六部分：CI/CD 构建矩阵（R2）](#第六部分cicd-构建矩阵r2)
- [第七部分：R2 落地路线图](#第七部分r2-落地路线图)
- [附录 A：R2 与 v0.0.1 差异对照](#附录-ar2-与-v001-差异对照)

---

## 第一部分：技术架构总则（R2 变更）

### 1.1 目标不变

功能目标（1.2/1.3 原规格）全部保留：5 平台、对标 PCL2/HMCL/FCL、插件系统、移动端触控、31 个功能模块。**只换实现语言与 UI 形态，不砍功能。**

### 1.2 R2 技术栈总览

| 项 | 选择 | 说明 |
| --- | --- | --- |
| 核心引擎语言 | **C（C11）为主** | 基础层/数据层/服务层主体 |
| 增强模块语言 | **Rust（中量）** | 下载网络栈、OAuth 认证、自更新、插件沙箱 |
| 平台热路径 | **汇编（少量）** | SHA-1/SHA-256 SIMD 加速（可选，后续里程碑） |
| 胶水层 | **C++（仅绑定）** | Rust/C 核心 → 各原生 UI 壳的桥接，无业务逻辑 |
| UI | **5 套平台原生** | 见第四部分 |
| 网络 | C：libcurl / Rust：reqwest | 下载模块在 Rust 侧 |
| 数据库 | SQLite（C API） | 账户存储 |
| JSON | C：内置轻量解析器 / Rust：serde_json | 各语言自带 |
| 构建 | CMake（C/C++/汇编）+ Cargo（Rust） | 双工具链，CI 分别验证 |
| 测试 | CTest（C）+ cargo test（Rust） | 模块测试各自语言内完成 |

### 1.3 分层结构（不变，语言标注更新）

| 层级 | 内容 | 语言 |
| --- | --- | --- |
| 第 1 层 | 基础层（日志、平台检测、工具、JSON） | C（+可选 asm SIMD） |
| 第 1.5 层 | 平台抽象垫片 platform/（进程、凭证、系统 UI） | C + 少量汇编 |
| 第 2 层 | 数据层（存储、配置、账户数据） | C |
| 第 3 层 | 服务层（下载、启动、版本、Java、账户、模组、插件、自更新） | C 为主，下载/认证/自更新/插件沙箱为 Rust |
| 第 4 层 | UI 层（5 套原生壳） | 各平台原生语言 + C++ 胶水 |
| 第 5 层 | 入口层 | 各平台原生入口 |

依赖规则、单一职责、可独立测试/替换原则（原 2.1/2.2）**全部保留**。同层通信通过接口：C 用头文件接口 + 函数指针/虚表，Rust 用 trait。

---

## 第二部分：语言构成与边界

### 2.1 语言分工（用户决策的落地）

```
┌─────────────────────────────────────────┐
│ UI 壳 ×5（原生）                         │  Win32/WinUI · AppKit/SwiftUI
│   Windows | macOS | Linux | Android | iOS │  GTK · Kotlin · Swift
├─────────────────────────────────────────┤
│ C++ 胶水层（仅绑定，无业务逻辑）          │  把 C ABI / Rust FFI 包成
│  rust_bridge / cpp_bridge                │  各 UI 语言可调用的 API
├─────────────────────────────────────────┤
│ Rust（中量）：DownloadService、AuthService│  reqwest/serde/tokio
│ SelfUpdater、Plugin 沙箱                 │  内存安全敏感区
├─────────────────────────────────────────┤
│ C（主要）：其余全部模块                  │  单一 C ABI，稳定可移植
├─────────────────────────────────────────┤
│ 汇编（少量）：SHA-1/SHA-256 SIMD         │  可选优化，隔离在 platform/asm/
└─────────────────────────────────────────┘
```

### 2.2 语言边界铁律

1. **C 是唯一主 ABI**：Rust 模块对外只暴露 `extern "C"` 接口；C++ 胶水只调 C ABI；各 UI 壳只调 C++ 胶水（或直接 C ABI）。
2. **Rust 不直接依赖 C 模块内部**：Rust 侧通过 C ABI 调用（如配置文件读取、版本数据），禁止 `#[link]` 到 C++ 符号。
3. **C++ 胶水零业务逻辑**：只做类型转换与调用转发；任何判断/计算逻辑都下沉到 C 或 Rust。
4. **汇编只允许出现在 platform/asm/**：必须提供 C 函数签名 + 纯 C fallback 实现，运行时按平台特性选择（cpuid 检测）。
5. **模块间通信必须跨 C ABI**：禁止 Rust/C++ 模块直接互相 include 头文件（除胶水层）。

### 2.3 目录规范（R2）

```
src/
├── core/        # C 基础层（logger, platform_detector, file_utils, string_utils, json）
├── data/        # C 数据层（account_data, account_storage, config_manager）
├── service/     # C 服务层（版本/安装/加载器/启动/Java/模组/插件管理器）
├── rust/        # Rust 模块（下载/认证/自更新/插件沙箱）+ FFI 导出
├── platform/    # C 平台垫片（process, credential, platform_ui）
│   └── asm/     # 少量汇编（sha1_sse2.s / sha256_avx2.s 等，含 C fallback）
├── cpp/         # C++ 胶水层（mkl_bridge.*）
├── ui/          # 5 套原生 UI 壳
│   ├── windows/  # Win32/WinUI (C++ 胶水 + 原生)
│   ├── macos/    # AppKit/SwiftUI (Swift)
│   ├── linux/    # GTK (C)
│   ├── android/  # Kotlin + Jetpack Compose
│   └── ios/      # Swift + UIKit
└── entry/       # 各平台入口（main）
tests/           # C 测试（test_*.c）
rust/*/tests/    # Rust 测试
```

---

## 第三部分：31 模块语言映射

> 功能、职责、接口语义均沿用 v0.0.1 规格；仅语言与接口形态变化。C 接口命名 `mkl_<模块>_<操作>`，Rust 模块用标准 Rust 惯例。

| 编号 | 模块 | 语言 | 接口形态 |
| --- | --- | --- | --- |
| 1 | Logger | C | `mkl_logger_log(level, file, line, msg, extra)` + 订阅回调 + 最近 50 行环形缓冲 |
| 2 | PlatformDetector | C | `mkl_platform_detect(mkl_platform_info*)` |
| 3 | FileUtils | C | `mkl_file_*` 系列；SHA-1 含 asm SIMD 可选 |
| 4 | StringUtils | C | `mkl_string_*` 系列 |
| 5 | JsonUtils | C | `mkl_json_get_string/int/bool/array/map` + 序列化 |
| 6 | AccountData | C | `mkl_account_data` struct + 枚举 |
| 7 | AccountStorage | C | SQLite C API + `mkl_credential_store` 平台垫片 |
| 8 | ConfigManager | C | `mkl_config_load/save/reset` + 热加载回调 |
| 9 | MinecraftVersionService | C | `mkl_version_*`（数据源 piston-meta + 镜像） |
| 10 | VersionInstaller | C | `mkl_installer_install/uninstall` + 进度回调 |
| 11 | ModLoaderInstaller | C | `mkl_modloader_install` + 版本范围表 |
| 12 | GameLauncher | C | `mkl_launcher_launch/stop/is_running` + 启动参数构造 |
| 13 | **DownloadService** | **Rust** | `extern "C"` 导出：`mkl_dl_add_task/pause/resume/cancel`（taskId）+ 进度回调 |
| 14 | JavaManager | C | `mkl_java_*`（8/17/21 按版本匹配） |
| 15 | **SelfUpdater** | **Rust** | `extern "C"` 导出：`mkl_update_check/download/apply/rollback` |
| 16 | **AuthService** | **Rust** | `extern "C"` 导出：微软 OAuth 设备码 + 刷新 + 离线 |
| 17 | ModManagerService | C | `mkl_mod_scan/set_enabled/delete` |
| 18 | PluginManager | C + **Rust 沙箱** | `.mklplugin` 解析 C；插件隔离执行 Rust 沙箱 |
| 19 | MainWindow | 原生×5 | 见第四部分 |
| 20 | TitleBar | 原生×5 | 各平台原生标题栏 |
| 21 | NavigationBar | 原生×5 | 各平台原生侧栏 |
| 22 | SettingsDialog | 原生×5 | 绑定 `IConfigService`（C 接口） |
| 23 | LogViewer | 原生×5 | 订阅 C Logger 回调 |
| 24 | HomePage | 原生×5 | 绑定服务接口 |
| 25 | VersionManagerPage | 原生×5 | 绑定服务接口 |
| 26 | ModManagerPage | 原生×5 | 绑定服务接口 |
| 27 | DownloadCenterPage | 原生×5 | 绑定 Rust 下载服务 FFI |
| 28 | TouchInput | 原生×2（Android/iOS） | 移动端触控（虚拟键/键盘贴图） |
| 29 | LayoutManager | 原生×5（薄封装） | 布局 JSON 由 C JsonUtils 解析，壳应用 |
| 30 | I18nManager | 原生×5（薄封装） | 各平台本地化机制 + C 配置联动 |
| 31 | ApplicationBootstrapper | 原生×5 入口 | 初始化 C/Rust 核心 → 注册服务 → 启动 UI |

### 3.1 C 接口风格示例（模块 8 ConfigManager）

```c
// src/data/config_manager.h
typedef struct mkl_app_config {
    char game_directory[512];
    char language[16];          /* "zh_CN" / "en_US" */
    char theme[16];             /* "light" / "dark" / "system" */
    int  isolated_versions;
    int  max_concurrent_downloads;
    int  download_speed_limit_kbps;   /* 0 = 不限 */
    char mirrors[4][256];
    int  mirror_count;
    char java_path[512];
    int  min_memory_mb;
    int  max_memory_mb;
    int  auto_download_java;
    char proxy_type[16];        /* "none" / "http" / "socks5" */
    char proxy_host[256];
    int  proxy_port;
    char proxy_username[128];
    char proxy_password[128];   /* 经 mkl_credential_store 加密 */
    char console_log_level[16];
    int  log_retention_days;
} mkl_app_config;

/* 返回 0 成功，非 0 为 mkl_error_code；详细错误写 err 结构 */
int mkl_config_load(mkl_app_config* out, mkl_error* err);
int mkl_config_save(const mkl_app_config* cfg, mkl_error* err);
void mkl_config_reset_to_defaults(mkl_app_config* cfg);
int mkl_config_subscribe(void (*on_change)(const mkl_app_config*), mkl_error* err);
```

### 3.2 Rust 接口风格示例（模块 13 DownloadService 的 FFI 导出）

```rust
// src/rust/mkl_download/src/ffi.rs —— 对外只暴露 extern "C"
#[repr(C)]
pub struct MklDownloadTask {
    pub task_id: [u8; 64],
    pub url: [u8; 1024],
    pub dest_path: [u8; 1024],
    pub priority: i32,           /* 0=HIGH 1=MEDIUM 2=LOW */
    pub expected_size: i64,      /* -1 = 未知 */
    pub expected_sha1: [u8; 64],
}

#[no_mangle]
pub extern "C" fn mkl_dl_add_task(task: *const MklDownloadTask,
                                  err: *mut MklError) -> *const c_char {
    // 返回 taskId（malloc 分配，调用方 mkl_free）
    // 内部：tokio 运行时 + reqwest 分片下载
}
```

---

## 第四部分：5 套原生 UI 壳规范

### 4.1 壳层职责（各平台相同）

| 职责 | 说明 |
| --- | --- |
| 渲染与交互 | 各平台原生控件，原生观感与性能 |
| 服务调用 | 只经 C++ 胶水层调用 C/Rust 核心；**UI 不直接触碰核心内部** |
| 页面结构 | 每个平台壳实现相同的 6 个页面（Home/VersionManager/ModManager/DownloadCenter/LogViewer/Settings）与导航 |
| 数据展示 | 核心以回调/轮询推送进度、日志、版本列表（统一 JSON 或 C 结构） |
| 平台能力 | 标题栏、系统托盘、通知、文件对话框、Keychain 等全部原生实现 |

### 4.2 平台选型

| 平台 | UI 技术 | 语言 | 与核心绑定 | 最低版本约束说明 |
| --- | --- | --- | --- | --- |
| Windows | Win32 API（自绘控件，对标 PCL2 风格） | C/C++ 胶水 | 直接调 C ABI | Win7 SP1 可用（WinUI 3 不支持 Win7，故用 Win32） |
| macOS | AppKit（SwiftUI 起步门槛高，壳阶段用 AppKit） | Swift | C++ 桥（Swift 调 C ABI 亦可） | macOS 14+ |
| Linux | GTK3 | C | 直接调 C ABI | GTK3 兼容 glibc 2.17 老发行版（GTK4 需要新 glibc，故 GTK3） |
| Android | Kotlin + Jetpack Compose（或传统 View） | Kotlin | JNI 桥 → C++ 胶水 | API 24+ |
| iOS | Swift + UIKit | Swift | 直接 C ABI（Swift 可调 C） | iOS 15+（实验性） |

### 4.3 UI 壳协议（跨壳一致性）

- **统一服务接口头文件** `cpp/mkl_bridge.h`：每个壳只需包含这一个头文件即可使用全部核心能力。
- **统一页面协议**：6 页面枚举 `mkl_page_type` 全平台一致；导航/页面切换语义一致。
- **统一事件协议**：日志/下载进度/账户状态以 C 回调结构体跨壳传递。
- **UI 壳不实现业务**：所有判断、计算、状态机在核心（C/Rust）；壳只做展示与交互转发。
- **触控（Android/iOS）**：壳内实现虚拟按键/触控板/键盘贴图（模块 28），输入经胶水层转发给游戏启动会话。

### 4.4 4.x 样式

各壳使用平台原生主题 + 共享设计令牌（颜色/间距/字号来自统一 `resources/design_tokens.json`，由 C JsonUtils 读取）。深浅色跟随系统（`system`）/用户配置（`light`/`dark`）。

---

## 第五部分：错误处理与命名规范（R2）

### 5.1 C 错误处理

- 所有可能失败的操作返回 `int` 错误码（`mkl_error_code`），可选 `mkl_error*` 携带上下文（文件/函数/行号/系统 errno）。
- 错误信息必须包含上下文；核心日志记录错误、警告与关键路径。

```c
/* src/core/error.h */
typedef enum mkl_error_code {
    MKL_OK = 0,
    MKL_ERR_INVALID_OPTIONS,
    MKL_ERR_NOT_FOUND,
    MKL_ERR_IO,
    MKL_ERR_NETWORK,
    MKL_ERR_DOWNLOAD_FAILED,
    MKL_ERR_VERIFICATION_FAILED,
    MKL_ERR_JAVA_NOT_FOUND,
    MKL_ERR_PROCESS_FAILED,
    MKL_ERR_AUTH_FAILED,
    MKL_ERR_STORAGE,
    MKL_ERR_PLUGIN,
    MKL_ERR_UPDATE_FAILED,
    MKL_ERR_UNSUPPORTED,
    MKL_ERR_UNKNOWN
} mkl_error_code;

typedef struct mkl_error {
    mkl_error_code code;
    char message[512];
    char file[128];
    char function[128];
    int line;
    int system_errno;
} mkl_error;

#define MKL_ERROR_INIT(e, c, m) \
    do { (e)->code = (c); mkl_snprintf((e)->message, sizeof((e)->message), "%s", (m)); \
         mkl_snprintf((e)->file, sizeof((e)->file), "%s", __FILE__); \
         mkl_snprintf((e)->function, sizeof((e)->function), "%s", __func__); \
         (e)->line = __LINE__; (e)->system_errno = errno; } while (0)
```

### 5.2 命名规范

| 语言 | 规则 | 示例 |
| --- | --- | --- |
| C | 前缀 `mkl_` + snake_case；类型 `mkl_<模块>_<type>`；函数 `mkl_<模块>_<操作>` | `mkl_logger_log`, `mkl_app_config` |
| C 枚举 | `MKL_` 前缀 + 全大写 | `MKL_ERR_NOT_FOUND`, `MKL_LOG_LEVEL_INFO` |
| Rust | rustfmt 标准（snake_case 函数、CamelCase 类型）；FFI 导出必须 `extern "C"` + `#[no_mangle]` | `mkl_dl_add_task`（FFI 名保持 mkl_ 前缀） |
| C++ 胶水 | 沿用 v0.0.1 规范（大驼峰类、小驼峰方法） | `MklBridge::addDownloadTask()` |
| 汇编 | 文件 `.s`，符号带 `mkl_asm_` 前缀，必须提供 C fallback | `mkl_asm_sha1_sse2` |
| 原生 UI | 各平台惯例（Swift/Kotlin 标准命名） | — |

### 5.3 Rust 错误处理

- Rust 内部用 `Result<T, MklError>`；FFI 边界转换为 C 错误码 + 错误结构（不 panic 越过 FFI 边界，使用 `catch_unwind`）。

---

## 第六部分：CI/CD 构建矩阵（R2）

| 平台 | runner | C 核心验证 | Rust 模块验证 | 产物 |
| --- | --- | --- | --- | --- |
| Ubuntu 22.04 | ubuntu-22.04 | cmake+ctest（gcc/clang） | cargo test | .AppImage（UI 壳完成后） |
| Windows 10/11 | windows-2022 | cmake+ctest（MSVC/MinGW） | cargo test | .exe + .zip |
| Windows 7 兼容 | self-hosted（可选） | 旧工具链验证 | — | 验证 |
| macOS 14/15 | macos-14/15 | cmake+ctest | cargo test | .dmg |
| Linux glibc 2.17 | 老容器（CentOS 7） | 自编译工具链 | — | .AppImage（后续） |
| Fedora 39 | self-hosted | cmake+ctest | cargo test | .rpm |
| Android | ubuntu-latest | NDK 交叉编译 | cargo ndk test（后续） | .apk |
| iOS | macos-14 | 交叉编译（后续） | cargo ios（后续） | 未签名 .app |

> 当前阶段（R2 骨架）：CI 只跑 C（三平台 cmake+ctest）+ Rust（三平台 cargo test）双工具链验证；UI 壳产物随各平台壳里程碑加入。

---

## 第七部分：R2 落地路线图

| 阶段 | 内容 | 验收 |
| --- | --- | --- |
| R2-S1 核心骨架 | C 基础层 5 模块 + 测试；Rust 占位 crate + 测试；双工具链 CI 全绿 | CI 三平台全绿 |
| R2-S2 数据层 | C 账户存储（SQLite+加密）、配置管理 + 测试 | 测试全绿 |
| R2-S3 核心服务 | C 版本/安装/Java/启动/模组；Rust 下载/认证/自更新 + FFI | CLI 可完成版本下载与启动参数构造 |
| R2-S4 Windows 壳 | Win32 壳 + C++ 胶水，6 页面 | Windows 可启动游戏 |
| R2-S5 macOS/Linux 壳 | AppKit / GTK3 壳 | 桌面 3 平台齐 |
| R2-S6 移动端 | Android(Kotlin)/iOS(Swift) 壳 + 触控 + TouchUI Mod | Android 实机可玩 |
| R2-S7 插件与发布 | 插件系统（C + Rust 沙箱）、.mklplugin、官网、文档、Release 全矩阵 | 全平台产物 |

---

## 附录 A：R2 与 v0.0.1 差异对照

| 维度 | v0.0.1（已废弃） | R2（现行） |
| --- | --- | --- |
| 核心语言 | C++17（Qt 6） | **主要 C（C11）** + 中量 Rust + 少量汇编 |
| UI | Qt Quick/QML 单套跨平台 | **5 套平台原生 UI 壳** |
| 胶水 | 无独立层 | **C++ 胶水层（cpp/mkl_bridge.h）**，零业务逻辑 |
| 网络 | Qt Network | **Rust reqwest**（下载/认证/自更新） |
| JSON | C++ JsonUtils | **C JsonUtils**（内置解析器）+ Rust serde_json |
| 模块接口 | C++ 类/接口 | C 接口（mkl_*）/ Rust trait + extern "C" |
| 错误处理 | Result<T> | C 错误码 + mkl_error 结构 / Rust Result |
| 构建 | CMake + Qt | **CMake（C/C++/asm）+ Cargo（Rust）双工具链** |
| 测试 | CTest | CTest（C）+ cargo test（Rust） |
| 功能/模块清单 | 31 模块 | 31 模块（**不砍任何功能**，仅语言映射变化） |
| Windows 7 | Qt 5.15 兼容分支 | **Win32 原生（天然兼容 Win7）** |
| Linux 老发行版 | Qt 自编译 | **GTK3（兼容 glibc 2.17）** |
| 内存占用 | Qt 运行时较重 | 原生 UI + C 核心，**体积与内存显著下降**（符合 2GB 机器诉求） |

> 注：当前仓库中 v0.0.1 的 C++/Qt 代码将被替换；首个 v0.0.1-alpha Release（正在打包）保留作为历史快照。
