# 麦块启动器（MKL）完整开发规格书（修正版 v0.0.1）

> **版本**：0.0.1-alpha
> **性质**：本文件为《MKL 开发规格书（整理版）》的**修正版**，替代整理版成为后续开发的唯一依据。
> **修正原则**：保留全部功能意图，仅修正无法实现/自相矛盾/缺失的设计（问题清单见 `MKL_规格问题清单与修正建议.md`，全部 40 项问题已按确认的决策处理，修正对照见[附录 A](#附录-a修正对照表)）。
> **已确认的关键决策**（见[附录 C](#附录-c技术决策记录)）：Qt Quick 统一跨端 UI；Qt6 主分支 + Qt5.15.2 Win7 兼容分支；Linux 老容器全链自编译保 GLIBC 2.17；移动端自更新改为"检测 + 引导"；C 类缺口全部补充为正式模块。

---

## 目录

- [第一部分：项目总览](#第一部分项目总览)
- [第二部分：代码架构总则](#第二部分代码架构总则)
- [第三部分：完整模块化设计（31 个独立模块）](#第三部分完整模块化设计31-个独立模块)
- [第四部分：UI 层模块化规范](#第四部分ui-层模块化规范)
- [第五部分：功能清单汇总](#第五部分功能清单汇总)
- [第六部分：CI/CD 构建矩阵](#第六部分cicd-构建矩阵)
- [第七部分：交付物清单与里程碑](#第七部分交付物清单与里程碑)
- [附录 A：修正对照表](#附录-a修正对照表)
- [附录 B：平台兼容性验证表（M1 填充）](#附录-b平台兼容性验证表m1-填充)
- [附录 C：技术决策记录](#附录-c技术决策记录)

---

## 第一部分：项目总览

### 1.1 项目身份

| 项目 | 内容 |
| --- | --- |
| 中文名 | 麦块启动器 |
| 英文名 | MKL (MaiKuai Launcher) |
| GitHub 组织 | MKL-Team |
| 启动器仓库 | launcher |
| 官网仓库 | website |
| 版本号 | 0.0.1-alpha |

### 1.2 一句话定位

MKL 是一个能在 **Windows、macOS、Linux、Android、iOS** 上运行的 Minecraft Java 版启动器，功能对标 PCL2、HMCL、FCL，并增加全平台插件系统和移动端完整触控方案。

### 1.3 平台要求

| 平台 | 最低版本 | 架构 | 说明 |
| --- | --- | --- | --- |
| Windows | 10（主支持）/ 7 SP1（兼容分支） | x64 | Win7 由 Qt 5.15.2 兼容分支提供（见 1.4 / 6.1） |
| macOS | 14 Sonoma | x64 + ARM64（通用） | — |
| Linux | GLIBC 2.17 | x64 | 由老容器全链自编译产物提供（见 6.1） |
| Android | API 24 (7.0)（目标，需 M1 验证） | arm64-v8a | Qt 官方最低版本以验证结果为准，必要时调整并说明 |
| iOS | 15.0（**实验性**） | arm64 | 侧载发布；无 JIT 环境下 JVM 性能受限，验收标准单独放宽 |

> **iOS 说明（A4）**：iOS 禁止 JIT，OpenJDK 无官方 iOS 版。iOS 支持保留，定位"实验性/侧载"：需完成兼容 JVM 方案调研（AOT/解释执行），配套 `ios_sideload.md`；游戏可运行性不作为 M1–M3 验收项。
> **iOS 许可（D4）**：Qt for iOS 静态链接场景 LGPL 不适用，需 GPL 开源发布或商业许可，纳入合规检查清单。

### 1.4 技术栈总览

| 项 | 选择 | 说明 |
| --- | --- | --- |
| 主 UI 框架 | **Qt 6.8 LTS + Qt Quick / QML** | 一套 QML UI 覆盖 5 平台；Qt Quick Controls 2 提供主题 |
| Win7 兼容分支 | **Qt 5.15.2 + VS2019 工具链** | 仅桌面 UI 使用 QML 兼容子集，C++ 用 Qt 5.15 兼容 API，差异走 `MKL_QT5` 条件编译 |
| 网络 | Qt Network | 服务层内部使用，不新增层级 |
| 数据库 | SQLite | 账户存储 |
| JSON | 单头文件 JSON 库（如 nlohmann/json）或内置轻量解析器 | 基础层不引入链接依赖 |
| 构建 | CMake（≥3.21）+ Ninja | 每个模块独立 target，可独立编译/测试 |
| 测试 | CTest + 各模块 test_*.cpp | Mock 服务器/模拟 Java 用于服务层测试 |

---

## 第二部分：代码架构总则

### 2.1 模块化原则

每个模块必须满足以下条件：

- **单一职责**：一个模块只做一件事
- **接口清晰**：头文件只暴露必要的外部接口
- **依赖明确**：#include 不超过 3 层深度
- **可独立测试**：每个模块有对应的 test_*.cpp
- **可独立替换**：模块之间通过抽象接口通信

### 2.2 模块层次结构（5 层）

| 层级 | 名称 | 内容 | 依赖方向 |
| --- | --- | --- | --- |
| 第 1 层 | 基础层 | 日志、平台检测、工具类、JSON、基础类型/错误处理、**平台抽象适配器（core/ + platform/）** | 无依赖 |
| 第 2 层 | 数据层 | 数据结构、存储（含凭证加密）、配置 | 仅依赖基础层 |
| 第 3 层 | 服务层 | 下载、启动、版本、Java、账户认证、模组管理、插件系统、自更新 | 依赖数据层 + 基础层 |
| 第 4 层 | UI 层 | QML 组件与 C++ 控制器（窗口、控件、页面、触控、布局、i18n） | 依赖服务层接口 + 基础层接口 |
| 第 5 层 | 入口层 | main.cpp、组装器、服务注册表 | 依赖所有层 |

**依赖规则**：上层可以依赖下层，下层绝不能依赖上层。同层模块之间通过接口通信，不能直接依赖实现（B4）。
**平台抽象归属（B12）**：`platform/` 是基础层的子目录，其中的适配器（进程、凭证、系统 UI）属于第 1 层。

### 2.3 命名规范

| 类型 | 格式 | 示例 |
| --- | --- | --- |
| 类名 | 大驼峰 | GameLauncher |
| 接口类 | I + 大驼峰 | IDownloader |
| 方法名 | 小驼峰 | launchGame() |
| 私有方法 | _ + 小驼峰 | _parseVersionJson() |
| 成员变量 | m_ + 小驼峰 | m_currentVersion |
| 静态变量 | s_ + 小驼峰 | s_instance |
| 常量 | 全大写 + _ | MAX_RETRY_COUNT |
| 枚举 | 大驼峰 | LauncherState |
| 枚举值 | 全大写 + _ | STATE_READY |
| 文件名 | 大驼峰 + .h/.cpp | GameLauncher.cpp |
| QML 文件 | 大驼峰 + .qml | MainWindow.qml |
| 测试文件 | snake_case（例外，D6） | test_logger.cpp |

### 2.4 目录规范

```
src/
├── core/              # 基础层（工具、类型、错误处理）
├── platform/          # 平台抽象适配器（基础层子目录：进程/凭证/系统UI）
├── data/              # 数据层（存储、配置、账户数据）
├── service/           # 服务层（下载、启动、版本、Java、认证、模组、自更新）
├── plugin/            # 插件系统（PluginAPI.h、PluginManager、.mklplugin 规范）
├── ui/                # UI 层 C++ 控制器
├── ui/qml/            # UI 层 QML 视图组件
├── entry/             # 入口层（main.cpp、ApplicationBootstrapper、ServiceRegistry）
└── resources/         # 翻译(.ts/.qm)、主题、图标、键盘贴图、默认布局 JSON
tests/                 # 各模块 test_*.cpp
```

### 2.5 错误处理规范（B3/B18 修正）

- **统一策略**：工具类（FileUtils/StringUtils/JsonUtils）允许 `bool + 日志`；**数据层、服务层接口一律返回 `Result<T>`**，不返回裸 bool，不抛业务异常。
- 错误信息必须包含上下文（文件、函数、行号、系统错误码）。
- 日志必须记录错误、警告和关键路径信息。

```cpp
// core/Error.h —— 基础层公共类型（不占模块编号）
enum class ErrorCode {
    OK = 0, INVALID_OPTIONS, NOT_FOUND, IO_ERROR, NETWORK_ERROR,
    DOWNLOAD_FAILED, VERIFICATION_FAILED, JAVA_NOT_FOUND, PROCESS_FAILED,
    AUTH_FAILED, STORAGE_ERROR, PLUGIN_ERROR, UPDATE_FAILED, UNSUPPORTED, UNKNOWN
};

struct Error {
    ErrorCode code;
    std::string message;        // 含上下文描述
    std::string file;           // __FILE__
    std::string function;       // __FUNCTION__
    int line;                   // __LINE__
    int systemErrorCode = 0;    // errno / GetLastError
};

template <typename T> class Result {
public:
    static Result<T> ok(T value);
    static Result<T> fail(const Error& error);
    bool isOk() const;
    bool isError() const;
    const T& value() const;      // 仅 isOk 时有效
    const Error& error() const;  // 仅 isError 时有效
};

#define MKL_ERR(code, msg) Error{code, msg, __FILE__, __FUNCTION__, __LINE__, errno}
```

使用示例：

```cpp
Result<AppConfig> ConfigManager::load() {
    if (!FileUtils::exists(m_path)) {
        return Result<AppConfig>::fail(MKL_ERR(ErrorCode::NOT_FOUND, "配置文件不存在: " + m_path));
    }
    // ...
    return config;
}
```

---

## 第三部分：完整模块化设计（31 个独立模块）

> 组成：基础层 5 + 数据层 3 + 服务层 10 + UI 层 12 + 入口层 1 = 31。
> 平台抽象适配器（3 个）与基础类型（Error/LogLevel）不占编号，见 3.0 之后说明。

### 3.0 模块总览索引

| 编号 | 模块名 | 层级 | 文件 | 核心职责 | 主要依赖 | 测试 |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | Logger | 基础层 | core/Logger.h/.cpp + core/LogLevel.h | 异步日志、多输出、级别过滤、实时订阅、FATAL 弹窗 | 无 | tests/test_logger.cpp |
| 2 | PlatformDetector | 基础层 | core/PlatformDetector.h/.cpp | 平台 / 系统版本 / 架构 / GLIBC 检测 | 无 | tests/test_platform_detector.cpp |
| 3 | FileUtils | 基础层 | core/FileUtils.h/.cpp | 文件读写、目录、路径、校验 | 标准库 + 平台系统调用 | tests/test_file_utils.cpp |
| 4 | StringUtils | 基础层 | core/StringUtils.h/.cpp | 字符串处理、格式化 | 无 | tests/test_string_utils.cpp |
| 5 | JsonUtils | 基础层 | core/JsonUtils.h/.cpp | JSON 解析/序列化（单头文件库或内置） | 无链接依赖 | tests/test_json_utils.cpp |
| 6 | AccountData | 数据层 | data/AccountData.h | 账户数据结构与枚举 | 基础层 | — |
| 7 | AccountStorage | 数据层 | data/AccountStorage.h/.cpp | 账户 SQLite 存储、凭证加密、查询 | 基础层 + SQLite + ICredentialStore | tests/test_account_storage.cpp |
| 8 | ConfigManager | 数据层 | data/ConfigManager.h/.cpp | 配置读写、默认值、热加载（实现 IConfigService） | 基础层 | tests/test_config_manager.cpp |
| 9 | MinecraftVersionService | 服务层 | service/MinecraftVersionService.h/.cpp | 版本清单拉取（piston-meta+镜像）、缓存、查询 | 基础层 + 数据层 | tests/test_version_service.cpp |
| 10 | VersionInstaller | 服务层 | service/VersionInstaller.h/.cpp | 安装版本（client.jar、libraries、assets） | 服务层(13) + 数据层 + 基础层 | tests/test_version_installer.cpp |
| 11 | ModLoaderInstaller | 服务层 | service/ModLoaderInstaller.h/.cpp | 安装 Forge/Fabric/OptiFine/LiteLoader/Quilt（含版本范围校验） | 服务层(14,13) + 基础层 | tests/test_modloader_installer.cpp |
| 12 | GameLauncher | 服务层 | service/GameLauncher.h/.cpp | 启动参数构造、进程启动、输出捕获、崩溃诊断 | 服务层(14) + ProcessAdapter + 基础层 | tests/test_game_launcher.cpp（模拟 Java） |
| 13 | DownloadService | 服务层 | service/DownloadService.h/.cpp | 多线程分片下载、断点续传、镜像切换、限速（taskId 标识） | 基础层 + Qt Network | tests/test_download_service.cpp（Mock 服务器） |
| 14 | JavaManager | 服务层 | service/JavaManager.h/.cpp | 系统 Java 检测、按 MC 版本提供 JRE 8/17/21 | 服务层(13) + 平台 + 基础层 | tests/test_java_manager.cpp |
| 15 | SelfUpdater | 服务层 | service/SelfUpdater.h/.cpp | 更新检查、桌面全量更新、移动端检测引导、回滚 | 服务层(13) + 基础层 | tests/test_self_updater.cpp（模拟 API） |
| 16 | AuthService | 服务层 | service/AuthService.h/.cpp | 微软 OAuth 设备码登录、token 刷新、离线账户 | 服务层(13) + 数据层(7) + 基础层 | tests/test_auth_service.cpp（模拟 OAuth） |
| 17 | ModManagerService | 服务层 | service/ModManagerService.h/.cpp | 模组扫描、识别、启用/禁用、删除 | 基础层 + 数据层 | tests/test_mod_manager_service.cpp |
| 18 | PluginManager | 服务层 | plugin/PluginManager.h/.cpp + plugin/PluginAPI.h | 插件扫描/加载/启停/卸载，.mklplugin 格式 | 服务层接口 + 基础层 | tests/test_plugin_manager.cpp |
| 19 | MainWindow | UI 层 | ui/qml/MainWindow.qml + ui/MainWindowController.h/.cpp | 主窗口布局（标题栏+导航+内容区）与页面切换 | 服务层接口（仅接口） | — |
| 20 | TitleBar | UI 层 | ui/qml/TitleBar.qml | 自定义标题栏（图标、标题、窗口控制） | — | — |
| 21 | NavigationBar | UI 层 | ui/qml/NavigationBar.qml | 侧边栏导航（账户头像、页面按钮） | — | — |
| 22 | SettingsDialog | UI 层 | ui/qml/SettingsDialog.qml | 所有设置的显示和编辑 | 服务层接口（IConfigService） | — |
| 23 | LogViewer | UI 层 | ui/qml/LogViewer.qml | 实时日志显示、级别过滤、搜索、导出 | 基础层 ILogger（订阅） | — |
| 24 | HomePage | UI 层 | ui/qml/HomePage.qml | 首页（快捷启动、账户信息、公告） | 服务层接口 | — |
| 25 | VersionManagerPage | UI 层 | ui/qml/VersionManagerPage.qml | 版本列表/安装/加载器安装 | 服务层接口（9/10/11/14） | — |
| 26 | ModManagerPage | UI 层 | ui/qml/ModManagerPage.qml | 模组列表与启停 | 服务层接口（17） | — |
| 27 | DownloadCenterPage | UI 层 | ui/qml/DownloadCenterPage.qml | 下载任务列表与进度 | 服务层接口（13） | — |
| 28 | TouchInput | UI 层 | ui/qml/TouchInput.qml + ui/TouchInput.h/.cpp | 移动端触控（虚拟键/触控板/键盘贴图） | 平台 + 基础层 | — |
| 29 | LayoutManager | UI 层 | ui/LayoutManager.h/.cpp | 布局 JSON 解析/应用/保存/重置 | 基础层(5) + UI 控件标识 | tests/test_layout_manager.cpp |
| 30 | I18nManager | UI 层 | ui/I18nManager.h/.cpp | 多语言（QTranslator/.qm 加载与切换） | Qt 翻译 | tests/test_i18n_manager.cpp |
| 31 | ApplicationBootstrapper | 入口层 | entry/ApplicationBootstrapper.h/.cpp + entry/ServiceRegistry.h/.cpp | 参数解析、初始化、创建服务、注入依赖、创建窗口、事件循环 | 所有层 | — |

**平台抽象适配器（基础层 platform/，不占编号）**：

| 适配器 | 文件 | 职责 | 被谁使用 |
| --- | --- | --- | --- |
| ProcessAdapter | platform/ProcessAdapter.h/.cpp | 进程创建/监控/终止/输出捕获（QProcess 封装） | GameLauncher |
| CredentialStore | platform/CredentialStore.h/.cpp | 凭证加密存取（DPAPI/Keychain/libsecret/Keystore），实现 ICredentialStore | AccountStorage、ConfigManager |
| PlatformUI | platform/PlatformUI.h/.cpp | 系统级 UI 回调（FATAL 弹窗等），实现 IPlatformUI | Logger |

**基础类型（core/，不占编号）**：`Error/ErrorCode/Result<T>`（core/Error.h，见 2.5）、`LogLevel`（core/LogLevel.h）。

### 3.1 基础层模块（第 1 层，5 个模块）

#### 模块 1：Logger（日志模块）

- **文件**：src/core/Logger.h, src/core/Logger.cpp, src/core/LogLevel.h
- **职责**：异步日志记录，多输出目标，级别过滤，实时订阅，FATAL 弹窗（最近 50 行）
- **依赖**：无
- **测试**：tests/test_logger.cpp

```cpp
enum class LogLevel { TRACE, DEBUG, INFO, WARN, ERROR, FATAL };

class ILogger {
public:
    virtual ~ILogger() = default;
    virtual void log(LogLevel level, const std::string& file, int line,
                     const std::string& msg, const std::map<std::string, std::string>& extra = {}) = 0;
    virtual void setConsoleOutput(bool enable) = 0;
    virtual void setMinConsoleLevel(LogLevel level) = 0;
    virtual void setLogFile(const std::string& path) = 0;
    virtual void flush() = 0;
    virtual void subscribe(std::function<void(LogLevel, const std::string&)> sink) = 0; // LogViewer 实时订阅（B6）
    virtual std::vector<std::string> recentLines(int count) = 0;  // FATAL 弹窗最近 50 行
};

class Logger : public ILogger {
public:
    static Logger& instance();   // 全局默认；核心模块经构造函数注入 ILogger*（B16）
    void setFatalDialogHandler(std::function<void(const std::vector<std::string>& last50)> handler); // IPlatformUI 注入（B17）
};

// 日志宏（自动填充 file/line）
#define MKL_LOG(level, msg) Logger::instance().log(level, __FILE__, __LINE__, msg)
#define MKL_INFO(msg)  MKL_LOG(LogLevel::INFO,  msg)
#define MKL_WARN(msg)  MKL_LOG(LogLevel::WARN,  msg)
#define MKL_ERROR(msg) MKL_LOG(LogLevel::ERROR, msg)
#define MKL_FATAL(msg) MKL_LOG(LogLevel::FATAL, msg)   // 触发弹窗显示最近 50 行
```

**完成标准**：
- 日志文件生成于 `logs/MKL_日期.log`；按 `logRetentionDays` 清理
- `--console` 开启控制台输出（Windows 下 AllocConsole）
- `MKL_FATAL` 触发弹窗显示最近 50 行（弹窗由 IPlatformUI 注入实现）

#### 模块 2：PlatformDetector（平台检测模块）

- **文件**：src/core/PlatformDetector.h, src/core/PlatformDetector.cpp
- **职责**：检测操作系统、版本、架构、GLIBC 版本
- **依赖**：无
- **测试**：tests/test_platform_detector.cpp

```cpp
enum class OSType { Windows, macOS, Linux, Android, iOS, Unknown };

struct PlatformInfo {
    OSType os;
    std::string osVersion;
    std::string arch;          // "x64", "arm64"
    std::string glibcVersion;  // Linux only
    int androidApiLevel;       // Android only
};

class PlatformDetector {
public:
    static PlatformInfo detect();
    static bool isWindows7();
    static bool isMacOS14OrLater();
    static bool isMobile();    // Android || iOS（供 UI 分支判断）
};
```

**完成标准**：各平台返回值正确，Windows 7 检测准确。

#### 模块 3：FileUtils（文件工具模块）

- **文件**：src/core/FileUtils.h, src/core/FileUtils.cpp
- **职责**：文件读写、目录操作、路径处理、文件校验
- **依赖**：仅 C++ 标准库 + 平台系统调用
- **测试**：tests/test_file_utils.cpp

```cpp
class FileUtils {
public:
    static bool exists(const std::string& path);
    static bool createDirectories(const std::string& path);
    static std::string readAllText(const std::string& path);
    static bool writeAllText(const std::string& path, const std::string& content);
    static bool copyFile(const std::string& src, const std::string& dest);
    static bool moveFile(const std::string& src, const std::string& dest);
    static bool deleteFile(const std::string& path);
    static bool deleteDirectory(const std::string& path);
    static uint64_t getFileSize(const std::string& path);          // 修正：数值类型（B1）
    static std::string formatFileSize(uint64_t bytes);             // 新增：可读字符串 "1.2 MB"
    static std::string getFileSha1(const std::string& path);
    static std::string getTempDirectory();
    static std::string getAppDataDirectory();   // 跨平台用户目录
    static std::string joinPath(const std::vector<std::string>& parts);
};
```

#### 模块 4：StringUtils（字符串工具模块）

- **文件**：src/core/StringUtils.h, src/core/StringUtils.cpp
- **职责**：字符串处理、格式化（JSON 职责已移至 JsonUtils，B2）
- **依赖**：无
- **测试**：tests/test_string_utils.cpp

```cpp
class StringUtils {
public:
    static std::string trim(const std::string& s);
    static std::vector<std::string> split(const std::string& s, char delimiter);
    static std::string replace(const std::string& s, const std::string& from, const std::string& to);
    static bool startsWith(const std::string& s, const std::string& prefix);
    static bool endsWith(const std::string& s, const std::string& suffix);
    static std::string toLower(const std::string& s);
    static std::string toUpper(const std::string& s);
    static std::string format(const char* fmt, ...);
    static std::map<std::string, std::string> parseQueryString(const std::string& qs);
};
```

#### 模块 5：JsonUtils（JSON 工具模块，新增）

- **文件**：src/core/JsonUtils.h, src/core/JsonUtils.cpp
- **职责**：JSON 解析、序列化、路径查询（承接原 StringUtils 的 JSON 职责）
- **依赖**：单头文件 JSON 库（如 nlohmann/json）或内置轻量解析器，**不引入链接依赖**
- **测试**：tests/test_json_utils.cpp

```cpp
class JsonUtils {
public:
    static bool isJson(const std::string& s);
    static std::string getString(const std::string& json, const std::string& keyPath,
                                 const std::string& def = "");
    static int64_t getInt(const std::string& json, const std::string& keyPath, int64_t def = 0);
    static bool getBool(const std::string& json, const std::string& keyPath, bool def = false);
    static std::vector<std::string> getStringArray(const std::string& json, const std::string& keyPath);
    static std::map<std::string, std::string> getStringMap(const std::string& json, const std::string& keyPath);
    static std::string serializeObject(const std::map<std::string, std::string>& obj);
    static std::string serializeStringArray(const std::vector<std::string>& arr);
    // keyPath 用点号分隔，如 "arguments.game.0.value"
};
```

### 3.2 数据层模块（第 2 层，3 个模块）

#### 模块 6：AccountData（账户数据结构）

- **文件**：src/data/AccountData.h
- **职责**：定义账户数据结构和枚举类型
- **依赖**：基础层

```cpp
enum class AccountType { Offline, Microsoft, External };

struct AccountData {
    std::string id;              // UUID
    AccountType type;
    std::string username;
    std::string accessToken;
    std::string refreshToken;
    std::string clientToken;
    std::string skinUrl;
    std::string capeUrl;
    int64_t expireTime;          // 毫秒时间戳
};
```

#### 模块 7：AccountStorage（账户存储模块）

- **文件**：src/data/AccountStorage.h, src/data/AccountStorage.cpp
- **职责**：账户数据的 SQLite 存储、查询、**凭证透明加密**
- **依赖**：基础层（StringUtils, FileUtils, ICredentialStore）+ SQLite
- **测试**：tests/test_account_storage.cpp
- **修正**：接口全面 Result 化（B18）；token 经 `ICredentialStore` 加密存储（B7）

```cpp
class IAccountStorage {
public:
    virtual ~IAccountStorage() = default;
    virtual Result<void> save(const AccountData& account) = 0;   // 内部加密 accessToken/refreshToken
    virtual Result<void> remove(const std::string& id) = 0;
    virtual Result<AccountData> get(const std::string& id) = 0;  // 未找到 → ErrorCode::NOT_FOUND
    virtual Result<std::vector<AccountData>> getAll() = 0;
    virtual Result<void> setCurrent(const std::string& id) = 0;
    virtual Result<AccountData> getCurrent() = 0;
};

class AccountStorage : public IAccountStorage {
    // SQLite 实现；构造函数注入 ICredentialStore*
};
```

**凭证加密方案（B7）**：`ICredentialStore` 平台实现——Windows DPAPI、macOS Keychain、Linux libsecret（fallback：加密文件 + 派生密钥）、Android Keystore、iOS Keychain；`proxyPassword` 等敏感配置同样走该接口。

#### 模块 8：ConfigManager（配置管理模块）

- **文件**：src/data/ConfigManager.h, src/data/ConfigManager.cpp
- **职责**：启动器配置的读写、默认值、热加载；实现 `IConfigService` 供 UI 使用（B5）
- **依赖**：基础层（FileUtils, StringUtils, JsonUtils）
- **测试**：tests/test_config_manager.cpp
- **修正**：新增 IConfigService 接口 + Result 化（B18）；单例提供默认实例但入口层可创建独立实例注入（B5）

```cpp
struct AppConfig {
    std::string gameDirectory;
    std::string language;       // "zh_CN", "en_US"
    std::string theme;          // "light", "dark", "system"
    bool isolatedVersions;
    int maxConcurrentDownloads;
    int downloadSpeedLimit;     // KB/s，0 不限
    std::vector<std::string> mirrors;
    std::string javaPath;
    int minMemoryMB;
    int maxMemoryMB;
    bool autoDownloadJava;
    std::string proxyType;      // "none", "http", "socks5"
    std::string proxyHost;
    int proxyPort;
    std::string proxyUsername;
    std::string proxyPassword;  // 经 ICredentialStore 加密存储
    std::string consoleLogLevel;
    int logRetentionDays;
};

class IConfigService {          // UI/插件依赖此接口（B5）
public:
    virtual ~IConfigService() = default;
    virtual Result<AppConfig> load() = 0;
    virtual Result<void> save(const AppConfig& config) = 0;
    virtual void resetToDefaults() = 0;
    virtual void setHotReload(bool enable) = 0;
    virtual void subscribe(std::function<void(const AppConfig&)> onChange) = 0; // 热加载通知
};

class ConfigManager : public IConfigService {
public:
    static ConfigManager& instance();   // 默认实例
    // 实现 IConfigService；JSON 存储于 getAppDataDirectory()/config.json
};
```

### 3.3 服务层模块（第 3 层，10 个模块）

#### 模块 9：MinecraftVersionService（版本服务模块）

- **文件**：src/service/MinecraftVersionService.h, src/service/MinecraftVersionService.cpp
- **职责**：Minecraft 版本清单拉取、缓存、查询（数据源 C9）
- **依赖**：基础层 + 数据层（mirrors 配置）
- **测试**：tests/test_version_service.cpp
- **修正**：明确数据源；接口异步化（B21）

**数据源（C9）**：
- 官方：`https://piston-meta.mojang.com/mc/game/version_manifest_v2.json`
- 镜像：BMCLAPI `https://bmclapi2.bangbang93.com/mc/game/version_manifest_v2.json`（按 `mirrors` 配置依次回退）
- 缓存：`versions/version_manifest_v2.json` + 本地版本目录扫描合并

```cpp
struct VersionInfo {
    std::string id;
    std::string type;           // "release", "snapshot", "old_alpha", "old_beta"
    std::string url;
    std::string releaseTime;
    std::string sha1;
};

class IVersionService {
public:
    virtual ~IVersionService() = default;
    virtual void fetchVersionList(std::function<void(Result<std::vector<VersionInfo>>)> callback) = 0; // 异步（B21）
    virtual Result<VersionInfo> getVersion(const std::string& id) = 0;   // 查本地缓存
    virtual void refreshCache() = 0;
};
```

#### 模块 10：VersionInstaller（版本安装模块）

- **文件**：src/service/VersionInstaller.h, src/service/VersionInstaller.cpp
- **职责**：下载并安装指定版本的 Minecraft 客户端（version json、client.jar、libraries、assets）
- **依赖**：服务层（IDownloadService）+ 数据层 + 基础层
- **测试**：tests/test_version_installer.cpp
- **修正**：percent 语义定义（B13）；安装流程细化

```cpp
struct InstallProgress {
    float percent;              // 0–100（B13）
    std::string currentFile;
    int downloadedFiles;
    int totalFiles;
};

struct InstallOptions {
    std::string versionId;
    std::string gameDir;
    bool isolated;
};

class IVersionInstaller {
public:
    virtual ~IVersionInstaller() = default;
    virtual void install(const InstallOptions& options,
                         std::function<void(const InstallProgress&)> progress,
                         std::function<void(Result<void>)> done) = 0;   // 异步（B21）
    virtual void uninstall(const std::string& versionId,
                           std::function<void(Result<void>)> done) = 0;
};
```

**安装流程**：拉取版本 JSON（`versions/<id>/<id>.json`）→ 解析 libraries（含 rules 过滤）与 assetIndex → 依次下载 client.jar / libraries / assets（走 IDownloadService，expectedSize/expectedSha1 校验，镜像回退）→ 写入版本目录。

#### 模块 11：ModLoaderInstaller（模组加载器安装模块）

- **文件**：src/service/ModLoaderInstaller.h, src/service/ModLoaderInstaller.cpp
- **职责**：安装 Forge、Fabric、OptiFine、LiteLoader、Quilt
- **依赖**：服务层（IJavaManager, IDownloadService——接口，B4）+ 基础层
- **测试**：tests/test_modloader_installer.cpp
- **修正**：支持版本范围元数据（B10）；OptiFine 独立流程

```cpp
enum class ModLoaderType { Forge, Fabric, OptiFine, LiteLoader, Quilt };

struct ModLoaderInstallOptions {
    ModLoaderType type;
    std::string minecraftVersion;
    std::string loaderVersion;  // 为空时自动选择推荐版本
    std::string gameDir;
};

class IModLoaderInstaller {
public:
    virtual ~IModLoaderInstaller() = default;
    virtual void install(const ModLoaderInstallOptions& options,
                         std::function<void(const std::string&)> output,
                         std::function<void(Result<void>)> done) = 0;
    virtual Result<ModLoaderSupportRange> getSupportRange(ModLoaderType type) = 0;
};
```

**支持范围（B10，实现时维护）**：Forge 1.1–最新；Fabric 1.14–最新；Quilt 1.18.2+；OptiFine 1.7.2–最新（**独立安装流程**，不进入 Forge 合并，冲突时提示）；LiteLoader 1.5.2–**1.12.2**（超出范围明确提示"已停止维护，不支持该版本"并禁用安装）。

#### 模块 12：GameLauncher（游戏启动模块）

- **文件**：src/service/GameLauncher.h, src/service/GameLauncher.cpp
- **职责**：构造启动参数，启动游戏进程，捕获输出，崩溃诊断
- **依赖**：服务层（IJavaManager）+ ProcessAdapter（平台）+ 基础层
- **测试**：tests/test_game_launcher.cpp（使用模拟 Java）
- **修正**：补启动参数构造流程（B14）与崩溃诊断细节（C8）

```cpp
struct LaunchOptions {
    std::string versionId;
    std::string accountId;
    std::string gameDir;
    std::string javaPath;
    int minMemoryMB;
    int maxMemoryMB;
    std::vector<std::string> extraJvmArgs;
    std::vector<std::string> extraGameArgs;
    bool closeLauncherAfterLaunch;
};

struct CrashReport {             // 崩溃诊断结果（C8）
    int exitCode;
    std::string summary;         // 分析结论与建议
    std::vector<std::string> evidenceFiles;  // crash-reports/ 与 hs_err_pid 路径
};

class IGameLauncher {
public:
    virtual ~IGameLauncher() = default;
    virtual Result<ProcessHandle> launch(const LaunchOptions& options) = 0;
    virtual void stop(ProcessHandle handle) = 0;
    virtual bool isRunning(ProcessHandle handle) = 0;
    virtual void setOutputCallback(std::function<void(const std::string&, bool isError)> callback) = 0;
    virtual std::optional<CrashReport> diagnoseCrash(ProcessHandle handle) = 0; // 退出后调用（C8）
};
```

**启动参数构造流程（B14）**：
1. 解析 `versions/<id>/<id>.json`：`arguments`（1.13+，含 rules/替换）或旧版 `minecraftArguments`（1.12-）
2. 解压 natives（libraries 中 `natives` 分类，Windows/macOS/Linux 各平台文件）到 `versions/<id>/natives-<hash>`
3. 解析 assets 索引（`assets/indexes/<assetIndex>.json`），建立对象路径映射
4. 组装 JVM 参数：`-Xmx/-Xms`、`-Djava.library.path=<natives>`、`-cp <client.jar:libraries...>`
5. 组装游戏参数：`--username/--uuid/--accessToken/--userType`（1.20.2+ 新格式 `--token/--uuid/--username/--userType` 兼容）、`--gameDir`、`--assetsDir`、`--version`、`--assetIndex`、`--versionType`
6. 经 ProcessAdapter 启动 `java` 进程，stdout/stderr 分流（isError）
7. 进程退出后执行崩溃诊断：退出码分类（0 正常 / 1 JVM 错误 / 负数信号）、检查 `crash-reports/` 最新文件与 `hs_err_pid*.log`，生成摘要与提示

#### 模块 13：DownloadService（下载服务模块）

- **文件**：src/service/DownloadService.h, src/service/DownloadService.cpp
- **职责**：多线程分片下载、断点续传、镜像切换、速度限制
- **依赖**：基础层（FileUtils, StringUtils）+ Qt Network（内部实现，B11）
- **测试**：tests/test_download_service.cpp（Mock 服务器）
- **修正**：taskId 标识（B8）、priority 枚举（B19）、进度回调（B21）

```cpp
enum class TaskPriority { HIGH, MEDIUM, LOW };          // B19

struct DownloadTask {
    std::string taskId;          // 由 addTask 生成，全生命周期唯一（B8）
    std::string url;
    std::string destPath;
    TaskPriority priority;
    std::optional<int64_t> expectedSize;
    std::optional<std::string> expectedSha1;
};

struct DownloadProgress {
    std::string taskId;
    std::string url;
    int64_t downloaded;
    int64_t total;
    int speed;                 // bytes per second
    int remainingSeconds;
};

struct DownloadOptions {
    int maxConcurrent;
    int speedLimitKBps;        // 0 不限
    std::vector<std::string> mirrorUrls;
};

class IDownloadService {
public:
    virtual ~IDownloadService() = default;
    virtual std::string addTask(const DownloadTask& task) = 0;            // 返回 taskId（B8）
    virtual void pauseTask(const std::string& taskId) = 0;
    virtual void resumeTask(const std::string& taskId) = 0;
    virtual void cancelTask(const std::string& taskId) = 0;
    virtual void pauseAll() = 0;
    virtual void resumeAll() = 0;
    virtual void setOptions(const DownloadOptions& options) = 0;
    virtual std::vector<DownloadProgress> getProgress() = 0;
    virtual void setProgressCallback(std::function<void(const DownloadProgress&)> cb) = 0; // 异步推送（B21）
};
```

**镜像切换**：主 URL 失败 → 按 `mirrorUrls` 依次回退；任务身份（taskId）不因切换改变。

#### 模块 14：JavaManager（Java 管理模块）

- **文件**：src/service/JavaManager.h, src/service/JavaManager.cpp
- **职责**：系统 Java 检测、按 MC 版本提供 JRE 8/17/21、自动下载（含 GLIBC 2.17 兼容）
- **依赖**：服务层（IDownloadService）+ 平台 + 基础层
- **测试**：tests/test_java_manager.cpp
- **修正**：按版本提供 JRE（B9）

```cpp
struct JavaInfo {
    std::string path;
    std::string version;
    std::string vendor;
    bool isJre;
};

class IJavaManager {
public:
    virtual ~IJavaManager() = default;
    virtual std::vector<JavaInfo> detectSystemJava() = 0;
    virtual Result<JavaInfo> findJavaFor(const std::string& minecraftVersion) = 0; // 8/17/21 匹配（B9）
    virtual Result<std::string> getOrDownloadJava(const std::string& majorVersion) = 0; // 返回 Java 路径
    virtual bool hasJava(const std::string& majorVersion) = 0;
    virtual void setCustomJavaPath(const std::string& path) = 0;
    virtual std::string getCurrentJavaPath() = 0;
};
```

**版本需求映射（B9）**：MC < 1.17 → Java 8；1.17 ≤ MC ≤ 1.20.4 → Java 17（默认）；MC ≥ 1.20.5 → Java 21。
**JRE 源**：Adoptium Temurin；Linux 产物需匹配 GLIBC 2.17（老容器自编译或选择兼容构建，见 6.1）。

#### 模块 15：SelfUpdater（自更新模块）

- **文件**：src/service/SelfUpdater.h, src/service/SelfUpdater.cpp
- **职责**：更新检查、桌面全量更新（下载/应用/回滚）、移动端检测引导
- **依赖**：服务层（IDownloadService）+ 基础层
- **测试**：tests/test_self_updater.cpp（模拟 GitHub API）
- **修正**：移动端改为"检测 + 引导"（A6）；通道抽象（A6）

```cpp
struct UpdateInfo {
    std::string version;
    std::string releaseUrl;
    std::map<std::string, std::string> assets;  // 平台键 -> downloadUrl；键格式如 "windows-x64"/"macos-arm64"/"linux-x64-glibc217"/"android-arm64"/"ios-arm64"（B15）
};

enum class UpdateChannelType { GitHub, AppStore, Sideload };

class IUpdateChannel {           // 通道可替换（A6）
public:
    virtual ~IUpdateChannel() = default;
    virtual Result<UpdateInfo> checkForUpdates() = 0;
    virtual Result<std::string> getDownloadUrl(const std::string& platformKey) = 0;
};

class ISelfUpdater {
public:
    virtual ~ISelfUpdater() = default;
    virtual Result<UpdateInfo> checkForUpdates() = 0;      // 桌面：GitHub Release；移动端：商店/侧载通道
    virtual Result<void> downloadUpdate(const UpdateInfo& info) = 0;   // 桌面
    virtual Result<void> applyUpdate() = 0;                // 桌面（重启后应用）
    virtual Result<void> rollback() = 0;                   // 桌面
    virtual void openStorePage(const UpdateInfo& info) = 0;// 移动端：跳转商店/侧载页（A6）
    virtual void setChannel(IUpdateChannel* channel) = 0;
};
```

#### 模块 16：AuthService（账户认证服务，新增）

- **文件**：src/service/AuthService.h, src/service/AuthService.cpp
- **职责**：微软 OAuth 设备码登录、token 自动刷新、离线账户创建/退出（C2）
- **依赖**：服务层（IDownloadService/网络）+ 数据层（IAccountStorage）+ 基础层
- **测试**：tests/test_auth_service.cpp（模拟 OAuth 服务器）

```cpp
class IAuthService {
public:
    virtual ~IAuthService() = default;
    virtual void loginMicrosoftDeviceCode(
        std::function<void(const std::string& userCode, const std::string& verifyUrl)> codeCallback,
        std::function<void(Result<AccountData>)> done) = 0;      // 设备码轮询（异步）
    virtual Result<AccountData> loginOffline(const std::string& username) = 0;
    virtual void refreshToken(const AccountData& account,
                              std::function<void(Result<AccountData>)> done) = 0;  // 过期自动刷新
    virtual Result<void> logout(const std::string& accountId) = 0;  // 移除账户并清理 token
};
```

**流程**：设备码（`https://login.microsoftonline.com/.../devicecode`）→ 用户浏览器授权 → 轮询 token 端点 → 换取 access/refresh token → 保存至 IAccountStorage（加密）；刷新失败 → AUTH_FAILED 并提示重新登录。

#### 模块 17：ModManagerService（模组管理服务，新增）

- **文件**：src/service/ModManagerService.h, src/service/ModManagerService.cpp
- **职责**：扫描 mods 目录、识别加载器类型、启用/禁用、删除（C4）
- **依赖**：基础层 + 数据层（配置的游戏目录）
- **测试**：tests/test_mod_manager_service.cpp

```cpp
struct ModInfo {
    std::string id;
    std::string name;
    std::string version;
    std::string loader;      // "forge", "fabric", "quilt", "unknown"
    std::string mcVersion;   // 目标 MC 版本（如可解析）
    std::string fileName;
    uint64_t fileSize;
    std::string sha1;
    bool enabled;
};

class IModManagerService {
public:
    virtual ~IModManagerService() = default;
    virtual Result<std::vector<ModInfo>> scanMods(const std::string& gameDir) = 0;
    virtual Result<void> setEnabled(const std::string& gameDir, const std::string& fileName, bool enabled) = 0;
    virtual Result<void> deleteMod(const std::string& gameDir, const std::string& fileName) = 0;
};
```

#### 模块 18：PluginManager（插件系统，新增）

- **文件**：src/plugin/PluginAPI.h（纯接口头文件）, src/plugin/PluginManager.h, src/plugin/PluginManager.cpp
- **职责**：插件扫描/加载/启用/停用/卸载；.mklplugin 打包格式（C1）
- **依赖**：服务层接口 + 基础层
- **测试**：tests/test_plugin_manager.cpp

```cpp
// PluginAPI.h —— 暴露给插件的接口（C1）
class IPluginContext {
public:
    virtual ~IPluginContext() = default;
    virtual ILogger* logger() = 0;
    virtual IConfigService* config() = 0;
    virtual IEventBus* events() = 0;                    // 事件总线（入口层提供）
    virtual std::string callService(const char* serviceId, const std::string& argsJson) = 0; // 服务调用桥
};

class IPlugin {
public:
    virtual ~IPlugin() = default;
    virtual const char* id() const = 0;
    virtual const char* version() const = 0;
    virtual const char* minMklVersion() const = 0;
    virtual bool onLoad(IPluginContext* ctx) = 0;       // 返回 false 则禁用该插件
    virtual void onUnload() = 0;
};

struct PluginDescriptor {
    std::string id;
    std::string version;
    std::string minMklVersion;
    std::string entryPoint;   // 动态库路径或 QML/脚本入口
    std::string description;
};

class IPluginManager {
public:
    virtual ~IPluginManager() = default;
    virtual Result<std::vector<PluginDescriptor>> scanPlugins(const std::string& dir) = 0;
    virtual Result<void> loadPlugin(const std::string& pluginId) = 0;
    virtual Result<void> unloadPlugin(const std::string& pluginId) = 0;
    virtual Result<void> setEnabled(const std::string& pluginId, bool enabled) = 0;
    virtual std::vector<std::string> getLoadedPlugins() = 0;
};
```

**.mklplugin 格式规范（C1）**：ZIP 容器，包含——
- `plugin.json`：`{ id, name, version, minMklVersion, entry, permissions[] }`
- `resources/`：QML 组件、图标等（UI 插件）
- 原生库：`lib/`（.dll / .so / .dylib，按平台）
- 可选 `signature`：插件签名（后续版本）
打包由 CI 脚本或 `mkl-plugin-pack` 工具生成；示例插件（空插件 + 打包示例）为交付物 7。

### 3.4 UI 层模块（第 4 层，12 个模块）

> **UI 层全部基于 Qt Quick / QML**（A1）。每个页面/控件 = QML 视图（ui/qml/*.qml）+ 需要逻辑时的 C++ 控制器（ui/*.h/.cpp）。桌面与移动端共用同一套 QML，观感差异由主题与平台分支控制。

#### 模块 19：MainWindow（主窗口模块）

- **文件**：src/ui/qml/MainWindow.qml + src/ui/MainWindowController.h, src/ui/MainWindowController.cpp
- **职责**：主窗口布局（TitleBar + NavigationBar + 内容 StackView）与页面切换
- **依赖**：所有 UI 组件、服务层接口（只依赖接口）；**容器例外**（B20：MainWindow 作为组装容器允许持有控件，控件之间互不依赖）
- **修正**：QML 化；页面切换经 Controller

```cpp
class MainWindowController : public QObject {
    Q_OBJECT
public:
    explicit MainWindowController(QObject* parent = nullptr);
    void init(IServiceRegistry* services);   // 依赖注入
    Q_INVOKABLE void switchToPage(int page); // PageType
signals:
    void navigationChanged(int page);        // 与 NavigationBar 同步（D3）
};
```

```qml
// MainWindow.qml
ApplicationWindow {
    TitleBar { id: titleBar; onMinimizeClicked: root.minimizeWindow(); ... }
    NavigationBar { id: navBar; onPageSelected: root.switchToPage(page) }
    StackView { id: contentStack; /* HomePage / VersionManagerPage / ... */ }
}
```

#### 模块 20：TitleBar（标题栏控件）

- **文件**：src/ui/qml/TitleBar.qml
- **职责**：自定义标题栏（图标、标题、最小化、最大化、关闭）
- **修正**：QML 化

```qml
// TitleBar.qml
Item {
    property string windowTitleText
    signal minimizeClicked
    signal maximizeClicked
    signal closeClicked
    signal doubleClicked      // 双击全屏切换
}
```

#### 模块 21：NavigationBar（导航栏控件）

- **文件**：src/ui/qml/NavigationBar.qml
- **职责**：侧边栏导航（账户头像、页面按钮）
- **修正**：QML 化

```qml
// NavigationBar.qml
Item {
    property int currentPage
    function setAccountInfo(username: string, avatar: url) {}
    signal pageSelected(int page)
    signal accountMenuRequested
}
```

#### 模块 22：SettingsDialog（设置对话框）

- **文件**：src/ui/qml/SettingsDialog.qml
- **职责**：所有设置的显示和编辑（含语言、主题、下载、Java、代理、日志）
- **依赖**：服务层接口（IConfigService）；语言切换联动 I18nManager（C7）
- **修正**：QML 化；依赖 IConfigService（B5）

#### 模块 23：LogViewer（日志查看器）

- **文件**：src/ui/qml/LogViewer.qml
- **职责**：实时日志显示，级别过滤，搜索，导出
- **依赖**：基础层 ILogger（订阅实时流，B6）
- **修正**：QML 化；经 `ILogger::subscribe` 接收实时日志

#### 模块 24：HomePage（首页，新增）

- **文件**：src/ui/qml/HomePage.qml
- **职责**：首页（当前账户、快捷启动、版本/Java 状态提示、公告）
- **依赖**：服务层接口（IAuthService / IVersionService / IJavaManager / IGameLauncher）

#### 模块 25：VersionManagerPage（版本管理页，新增）

- **文件**：src/ui/qml/VersionManagerPage.qml
- **职责**：已安装版本列表、安装新版本、加载器安装、版本卸载
- **依赖**：服务层接口（IVersionService / IVersionInstaller / IModLoaderInstaller / IJavaManager）

#### 模块 26：ModManagerPage（模组管理页，新增）

- **文件**：src/ui/qml/ModManagerPage.qml
- **职责**：模组列表、启用/禁用、删除
- **依赖**：服务层接口（IModManagerService）

#### 模块 27：DownloadCenterPage（下载中心页，新增）

- **文件**：src/ui/qml/DownloadCenterPage.qml
- **职责**：下载任务列表、进度/速度/剩余时间、暂停/恢复/取消
- **依赖**：服务层接口（IDownloadService，progress 回调）

#### 模块 28：TouchInput（触控输入，新增）

- **文件**：src/ui/qml/TouchInput.qml + src/ui/TouchInput.h, src/ui/TouchInput.cpp
- **职责**：移动端触控方案——虚拟按键、虚拟触控板/摇杆、键盘贴图渲染、输入转发（C5）
- **依赖**：平台 + 基础层
- **完成标准**：移动端可完整操作游戏（触屏视角 + 按键映射 + 键盘贴图）；与 TouchUI Mod 联动

```cpp
class TouchInput : public QObject {
    Q_OBJECT
public:
    explicit TouchInput(QObject* parent = nullptr);
    void attachToWindow(QQuickWindow* window);
    void setKeyboardTexture(const QString& path);      // 键盘贴图（交付物 6）
    void setLayoutJson(const QString& layoutJson);     // 按键布局（可自定义）
    Q_INVOKABLE void showKeyboard();
    Q_INVOKABLE void hideKeyboard();
    Q_INVOKABLE void setMode(int mode);                // 触控板 / 摇杆 / 直触
signals:
    void keyEvent(int qtKey, bool down);               // 转发到游戏输入
    void pointerMoved(const QPointF& pos);
    void pointerClicked(const QPointF& pos);
};
```

#### 模块 29：LayoutManager（布局管理，新增）

- **文件**：src/ui/LayoutManager.h, src/ui/LayoutManager.cpp
- **职责**：布局 JSON 解析/应用/保存/重置（默认布局 JSON 的承载方，C6）
- **依赖**：基础层（JsonUtils）+ UI 控件标识
- **测试**：tests/test_layout_manager.cpp

```cpp
class ILayoutManager {
public:
    virtual ~ILayoutManager() = default;
    virtual Result<void> applyLayout(const std::string& json) = 0;
    virtual Result<std::string> exportLayout() = 0;
    virtual Result<void> saveDefaultLayout(const std::string& json) = 0;  // 写入 resources/default_layout.json
    virtual Result<void> resetToDefault() = 0;
};
```

#### 模块 30：I18nManager（多语言管理，新增）

- **文件**：src/ui/I18nManager.h, src/ui/I18nManager.cpp
- **职责**：多语言（QTranslator 加载 .qm、语言切换、UI 文案重载）（C7）
- **依赖**：Qt 翻译（resources/translations/*.ts → .qm）
- **测试**：tests/test_i18n_manager.cpp

```cpp
class I18nManager : public QObject {
    Q_OBJECT
public:
    static I18nManager& instance();
    Q_INVOKABLE QString tr(const QString& key) const;   // 包装 QObject::tr / QTranslator
    Result<void> setLanguage(const QString& lang);      // "zh_CN" / "en_US"
    QString currentLanguage() const;
signals:
    void languageChanged(const QString& lang);
};
```

### 3.5 入口层模块（第 5 层，1 个模块）

#### 模块 31：ApplicationBootstrapper（应用启动组装器）

- **文件**：src/entry/ApplicationBootstrapper.h, src/entry/ApplicationBootstrapper.cpp + src/entry/ServiceRegistry.h, src/entry/ServiceRegistry.cpp
- **职责**：
  - 解析命令行参数（`--console` 等）
  - 初始化日志系统（含 IPlatformUI 注入）
  - 创建所有服务实例（AccountStorage、ConfigManager、DownloadService、JavaManager、AuthService、VersionService、VersionInstaller、ModLoaderInstaller、ModManagerService、GameLauncher、SelfUpdater、PluginManager…）
  - 注入依赖（服务经 `IServiceRegistry` 暴露给 UI 与插件）
  - 创建 QML 引擎与主窗口、加载主题/翻译/默认布局
  - 运行事件循环
- **依赖**：所有层所有模块

```cpp
// ServiceRegistry.h —— 服务注册表（依赖注入容器）
class IServiceRegistry {
public:
    virtual ~IServiceRegistry() = default;
    virtual void registerService(const char* id, void* service) = 0;
    virtual void* getService(const char* id) = 0;      // 使用前 static_cast 到对应接口
};
```

---

## 第四部分：UI 层模块化规范

### 4.1 UI 模块依赖原则（B20 修正）

- UI 层模块只依赖服务层的**抽象接口**与基础层的**抽象接口**（不依赖具体类）
- UI 层**控件之间**不能互相依赖（每个控件独立）；**MainWindow 作为组装容器是唯一例外**，允许持有子控件
- UI 层模块通过**信号/槽（QML signals）通信**，不直接调用对方方法
- 页面切换同步规则（D3）：`NavigationBar.pageSelected` 是唯一触发源，`MainWindowController.switchToPage` 的结果通过 `navigationChanged` 通知回写，防止循环

### 4.2 页面枚举与映射表（D2）

```cpp
enum class PageType {
    Home,           // → HomePage（模块 24）
    VersionManager, // → VersionManagerPage（模块 25）
    ModManager,     // → ModManagerPage（模块 26）
    DownloadCenter, // → DownloadCenterPage（模块 27）
    LogViewer,      // → LogViewer（模块 23，作为页面嵌入 StackView）
    Settings        // → SettingsDialog（模块 22，作为页面或对话框）
};
```

| PageType | 页面模块 | 导航入口 |
| --- | --- | --- |
| Home | HomePage | 首页按钮 |
| VersionManager | VersionManagerPage | 版本管理按钮 |
| ModManager | ModManagerPage | 模组管理按钮 |
| DownloadCenter | DownloadCenterPage | 下载中心按钮 |
| LogViewer | LogViewer | 日志按钮 |
| Settings | SettingsDialog | 设置按钮 |

### 4.3 样式规范（A1 修正：QSS → Qt Quick Controls 2 主题）

```cpp
// 所有 UI 模块通过统一的主题管理器加载主题
class ThemeManager {
public:
    static void applyTheme(const QString& theme);   // "light" / "dark" / "system"
    static QString currentTheme();
    static void registerQml(const QString& theme, const QString& qmlStyleUrl); // 主题 QML 样式
};
```

- 主题（浅色/深色/跟随系统）以 Qt Quick Controls 2 样式表（`Theme.qml`）+ 调色板实现
- 主题切换联动 `AppConfig.theme` 与 SettingsDialog

---

## 第五部分：功能清单汇总

### 5.1 所有功能模块汇总（31 个）

| 编号 | 模块名 | 层级 | 职责 |
| --- | --- | --- | --- |
| 1 | Logger | 基础层 | 异步日志、订阅、FATAL 弹窗 |
| 2 | PlatformDetector | 基础层 | 平台检测 |
| 3 | FileUtils | 基础层 | 文件操作 |
| 4 | StringUtils | 基础层 | 字符串处理 |
| 5 | JsonUtils | 基础层 | JSON 解析/序列化 |
| 6 | AccountData | 数据层 | 账户数据结构 |
| 7 | AccountStorage | 数据层 | 账户存储（含凭证加密） |
| 8 | ConfigManager | 数据层 | 配置管理（IConfigService） |
| 9 | MinecraftVersionService | 服务层 | 版本清单与缓存 |
| 10 | VersionInstaller | 服务层 | 版本安装 |
| 11 | ModLoaderInstaller | 服务层 | 加载器安装（含范围校验） |
| 12 | GameLauncher | 服务层 | 游戏启动与崩溃诊断 |
| 13 | DownloadService | 服务层 | 下载管理 |
| 14 | JavaManager | 服务层 | Java 管理（8/17/21） |
| 15 | SelfUpdater | 服务层 | 自更新（桌面全量/移动引导） |
| 16 | AuthService | 服务层 | 账户认证（微软 OAuth/离线） |
| 17 | ModManagerService | 服务层 | 模组管理 |
| 18 | PluginManager | 服务层 | 插件系统（.mklplugin） |
| 19 | MainWindow | UI 层 | 主窗口与页面切换 |
| 20 | TitleBar | UI 层 | 标题栏 |
| 21 | NavigationBar | UI 层 | 导航栏 |
| 22 | SettingsDialog | UI 层 | 设置对话框 |
| 23 | LogViewer | UI 层 | 日志查看器 |
| 24 | HomePage | UI 层 | 首页 |
| 25 | VersionManagerPage | UI 层 | 版本管理页 |
| 26 | ModManagerPage | UI 层 | 模组管理页 |
| 27 | DownloadCenterPage | UI 层 | 下载中心页 |
| 28 | TouchInput | UI 层 | 移动端触控 |
| 29 | LayoutManager | UI 层 | 布局管理 |
| 30 | I18nManager | UI 层 | 多语言 |
| 31 | ApplicationBootstrapper | 入口层 | 启动组装与依赖注入 |

---

## 第六部分：CI/CD 构建矩阵（A2/A3/A4/A5 修正）

| 平台 | runner / 环境 | 工具链 | 产物 | 说明 |
| --- | --- | --- | --- | --- |
| Windows 10/11 | windows-2025 | MSVC + Qt 6.8 | .exe + .zip | 主分支 |
| Windows 7 兼容分支 | windows-2022 | **VS2019 (v142) + Qt 5.15.2** | .exe + .zip | 与主分支同源，`MKL_QT5` 条件编译（A2） |
| Windows 7 真机验证 | self-hosted（可选） | — | 验证 | 启动/安装/启动游戏冒烟 |
| macOS 14/15 | macos-14, macos-15 | Xcode + Qt 6.8 | .dmg | x64 + arm64 通用 |
| Linux（通用） | ubuntu-22.04 | GCC + Qt 6.8 | .AppImage + .deb | glibc 2.35 基线 |
| Linux GLIBC 2.17 | **老容器（CentOS 7）** | 自编译 Qt/OpenSSL/curl | .AppImage | 全链自编译，满足 GLIBC 2.17（A3）；JRE 源同样需 2.17 兼容 |
| Fedora 39 | self-hosted | GCC + Qt 6.8 | .rpm | — |
| Android | ubuntu-latest | Qt 6.8 + Android SDK/NDK | .apk | 最低 API 以 Qt 官方矩阵为准，M1 验证后定稿（A5） |
| iOS | macos-14 | Xcode + Qt for iOS（GPL/商业） | 未签名 .app | 实验性，侧载（A4/D4） |

**双版本兼容约束（A2）**：QML 仅使用 Qt 5.15 与 Qt 6 共有的 API 子集；C++ 使用 Qt 5.15+ 兼容 API，`Qt6` 独有特性以 `#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)` / `MKL_QT5` 宏隔离；UI 自动化测试在两条工具链上各跑一遍。

---

## 第七部分：交付物清单与里程碑

### 7.1 交付物清单

1. 启动器完整源码（31 个模块独立文件，按目录组织）
2. TouchUI Mod 源码（独立 JAR 项目）
3. 官网完整源码（Next.js）
4. GitHub Actions 工作流（build.yml, test-compatibility.yml, deploy.yml）
5. 文档（README.md, user_manual.md, developer_guide.md, plugin_development.md, ios_sideload.md）
6. 资源文件（图标集、主题、默认布局 JSON、键盘贴图、翻译 .ts/.qm）
7. 示例插件（空插件源码 + .mklplugin 打包示例）

### 7.2 里程碑拆分（D5）

| 里程碑 | 内容 | 验收 |
| --- | --- | --- |
| M1 桌面 MVP | 基础层 + 数据层 + 核心服务（下载/版本/Java/启动）+ 桌面 UI 骨架 | Windows/macOS/Linux 可离线/正版启动 1.20.x |
| M2 功能完善 | 账户认证（微软 OAuth）、版本安装/加载器、模组管理、下载中心、设置、日志 | 对标 PCL2/HMCL 桌面核心功能 |
| M3 插件系统 | PluginManager/PluginAPI/.mklplugin 打包与示例插件 | 示例插件可加载运行 |
| M4 移动端 | Qt Quick 移动 UI、TouchInput 触控、TouchUI Mod | Android 实机可玩；iOS 实验版侧载包 |
| M5 发布 | 自更新、官网、文档、CI 全矩阵、商店/侧载渠道 | 全平台产物 + 发布流程 |

> **总要求**：严格按照以上规格生成完整的项目代码、配置文件和文档。所有模块必须独立、清晰、可测试。

---

## 附录 A：修正对照表

| 编号 | 修正内容 | 修正位置 |
| --- | --- | --- |
| A1 | UI 改为 Qt Quick/QML 统一跨端；QSS → Qt Quick Controls 2 主题 | 1.4、3.4、4.1、4.3 |
| A2 | Qt6 主分支 + Qt5.15.2 Win7 兼容分支，`MKL_QT5` 条件编译 | 1.3、1.4、6.1 |
| A3 | 老容器（CentOS 7）全链自编译保 GLIBC 2.17 | 1.3、6.1、模块 14 备注 |
| A4 | iOS 定位实验性/侧载，验收标准单独放宽 | 1.3 |
| A5 | 锁定 Qt 6.8 LTS，Android 最低版本 M1 验证后定稿 | 1.3、6.1、附录 B |
| A6 | SelfUpdater：桌面全量更新 + 移动端检测引导，IUpdateChannel 抽象 | 模块 15 |
| B1 | `getFileSize` → `uint64_t` + `formatFileSize` | 模块 3 |
| B2 | JSON 职责移入新增 JsonUtils | 模块 4、5 |
| B3 | 新增 Error/ErrorCode/Result/LogLevel 定义，统一错误策略 | 2.5、模块 1 |
| B4 | 同层依赖改接口（IJavaManager / IDownloadService） | 模块 11、12、14 |
| B5 | 新增 IConfigService 接口，ConfigManager 实现之 | 模块 8、22 |
| B6 | LogViewer 标注为基础层 ILogger 依赖 + subscribe 订阅 | 模块 1、23 |
| B7 | 新增 ICredentialStore 平台凭证存储，账户/配置透明加密 | 模块 7、平台适配器 |
| B8 | 下载任务改 taskId 标识 | 模块 13 |
| B9 | JavaManager 按 MC 版本提供 JRE 8/17/21 | 模块 14 |
| B10 | ModLoader 支持版本范围元数据；OptiFine 独立流程 | 模块 11 |
| B11 | 网络层明确为 DownloadService 内部实现 | 模块 13、2.2 |
| B12 | platform/ 明确为基础层子目录 | 2.2、2.4 |
| B13 | InstallProgress.percent 定义 0–100 | 模块 10 |
| B14 | 补"启动参数构造流程"（arguments/natives/assets/JVM/游戏参数/诊断） | 模块 12 |
| B15 | UpdateInfo.assets 平台键格式定义 | 模块 15 |
| B16 | Logger instance() + 构造注入双轨 + LOG_* 宏 | 模块 1 |
| B17 | FATAL 弹窗经 IPlatformUI 注入；最近 50 行环形缓冲 | 模块 1、平台适配器 |
| B18 | 数据/服务层接口 Result 化，工具类 bool + 日志 | 2.5、模块 7、8 |
| B19 | DownloadTask.priority 改 TaskPriority 枚举 | 模块 13 |
| B20 | MainWindow 容器例外规则 | 4.1、模块 19 |
| B21 | 服务层接口异步化（回调/任务队列） | 模块 9、10、11、13、15、16 |
| C1 | 新增 PluginManager/PluginAPI/.mklplugin 规范 | 模块 18 |
| C2 | 新增 AuthService（微软 OAuth/离线） | 模块 16 |
| C3 | 新增 4 个页面模块 | 模块 24–27 |
| C4 | 新增 ModManagerService | 模块 17 |
| C5 | 新增 TouchInput | 模块 28 |
| C6 | 新增 LayoutManager | 模块 29 |
| C7 | 新增 I18nManager | 模块 30 |
| C8 | GameLauncher 补崩溃诊断细节 | 模块 12 |
| C9 | 版本清单数据源（piston-meta + 镜像） | 模块 9 |
| D1 | 模块计数 19→20→31；UI 层 5 个修正 | 第三部分 |
| D2 | PageType 与页面映射表 | 4.2 |
| D3 | 页面同步防循环规则 | 4.1 |
| D4 | Qt for iOS 许可说明 | 1.3 |
| D5 | 交付物拆里程碑 M1–M5 | 7.2 |
| D6 | 测试文件命名例外 | 2.3 |

## 附录 B：平台兼容性验证表（M1 填充）

| 平台 | Qt 版本 | 工具链 | 最低版本声明 | 验证项（M1 完成） | 状态 |
| --- | --- | --- | --- | --- | --- |
| Windows 10/11 | 6.8 | MSVC | Win10 1809+ | 安装/启动/下载/启动游戏 | 待验证 |
| Windows 7 | 5.15.2 | VS2019 | Win7 SP1 | 同上 + 真实 Win7 冒烟 | 待验证 |
| macOS 14/15 | 6.8 | Xcode | macOS 14 | x64/arm64 双架构 | 待验证 |
| Linux glibc 2.17 | 6.8（自编译） | GCC（CentOS 7 容器） | glibc 2.17 | 老发行版实机 | 待验证 |
| Android | 6.8 | NDK | 以官方矩阵为准（目标 API 24） | API 24 真机 | 待验证 |
| iOS | 6.8 | Xcode | iOS 15 | 侧载安装 + JVM 性能基准 | 待验证 |

## 附录 C：技术决策记录

| 决策 | 结论 |
| --- | --- |
| 问题清单处理方式 | 出《规格书修正版》，按"保留功能意图、修正设计实现"逐项修正 |
| UI 技术栈（A1） | Qt Quick/QML 统一跨端，一套 UI 覆盖 5 平台 |
| Windows 7（A2） | 双构建：Qt6 主分支 + Qt5.15.2 兼容分支 |
| GLIBC 2.17（A3） | 老容器全链自编译，产出真正兼容产物 |
| 移动端自更新（A6） | 桌面自更新 + 移动端检测引导（IUpdateChannel） |
| 缺口模块（C 类） | 全部补充为正式模块，模块总数 20 → 31 |
