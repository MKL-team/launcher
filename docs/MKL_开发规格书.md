# 麦块启动器（MKL）完整开发规格书（整理版）

> **版本**：0.0.1-alpha
> **来源**：《MKL 最终完整开发提示词（含超颗粒模块化规范）》
> **说明**：本文件为原规格的整理版，内容保持原规格不变，仅修正编号笔误、统一格式、补充目录与模块索引，作为后续开发与评审的唯一依据。整理修正记录见[附录 A](#附录-a整理修正记录)。

---

## 目录

- [第一部分：项目总览](#第一部分项目总览)
- [第二部分：代码架构总则](#第二部分代码架构总则)
- [第三部分：完整模块化设计（20 个独立模块）](#第三部分完整模块化设计20-个独立模块)
- [第四部分：UI 层模块化规范](#第四部分ui-层模块化规范)
- [第五部分：功能清单汇总](#第五部分功能清单汇总)
- [第六部分：CI/CD 构建矩阵](#第六部分cicd-构建矩阵)
- [第七部分：交付物清单](#第七部分交付物清单)
- [附录 A：整理修正记录](#附录-a整理修正记录)
- [附录 B：规格中待补充定义的类型](#附录-b规格中待补充定义的类型)

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

| 平台 | 最低版本 | 架构 |
| --- | --- | --- |
| Windows | 7 SP1 | x64 |
| macOS | 14 Sonoma | x64 + ARM64（通用） |
| Linux | GLIBC 2.17 | x64 |
| Android | API 24 (7.0) | arm64-v8a |
| iOS | 15.0 | arm64 |

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
| 第 1 层 | 基础层 | 日志、平台兼容、工具类 | 无依赖 |
| 第 2 层 | 数据层 | 数据结构、存储、配置 | 仅依赖基础层 |
| 第 3 层 | 服务层 | 下载、启动、版本管理、账户 | 依赖数据层 + 基础层 |
| 第 4 层 | UI 层 | 窗口、控件、视图 | 依赖服务层 |
| 第 5 层 | 入口层 | main.cpp，组装所有模块 | 依赖所有层 |

**依赖规则**：上层可以依赖下层，下层绝不能依赖上层。同层模块之间通过接口通信，不能直接依赖实现。

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

### 2.4 目录规范

```
src/
├── core/              # 基础层
├── data/              # 数据层（存储、配置、账户数据）
├── service/           # 服务层（下载、启动、版本、Java）
├── ui/                # UI 层（窗口、控件、视图）
├── plugin/            # 插件系统（接口、管理器）
├── platform/          # 平台抽象（适配器）
└── entry/             # 入口层（main.cpp、组装器）
```

### 2.5 错误处理规范

- 所有可能失败的操作必须返回 `Result<T>` 或抛出明确的异常类型
- 错误信息必须包含上下文（文件、函数、行号、系统错误码）
- 日志必须记录错误、警告和关键路径信息

```cpp
// 错误处理模板
Result<bool> GameLauncher::launch(const LaunchOptions& options) {
    if (!_validateOptions(options)) {
        return Error(ErrorCode::INVALID_OPTIONS, "内存大小超出范围");
    }
    // ...
    return true;
}
```

---

## 第三部分：完整模块化设计（20 个独立模块）

> 原规格标题为「19 个独立模块」，经核对编号为 1–20，实际共 **20 个模块**（基础层 4 + 数据层 3 + 服务层 7 + UI 层 5 + 入口层 1）。

### 3.0 模块总览索引

| 编号 | 模块名 | 层级 | 文件 | 核心职责 | 主要依赖 | 测试 |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | Logger | 基础层 | core/Logger.h/.cpp | 异步日志、多输出目标、级别过滤 | 无 | tests/test_logger.cpp |
| 2 | PlatformDetector | 基础层 | core/PlatformDetector.h/.cpp | 平台 / 系统版本 / 架构 / GLIBC 检测 | 无 | tests/test_platform_detector.cpp |
| 3 | FileUtils | 基础层 | core/FileUtils.h/.cpp | 文件读写、目录、路径、校验 | 标准库 + 平台系统调用 | tests/test_file_utils.cpp |
| 4 | StringUtils | 基础层 | core/StringUtils.h/.cpp | 字符串处理、JSON、格式化 | 无 | tests/test_string_utils.cpp |
| 5 | AccountData | 数据层 | data/AccountData.h | 账户数据结构与枚举 | 基础层 | — |
| 6 | AccountStorage | 数据层 | data/AccountStorage.h/.cpp | 账户 SQLite 存储、查询、加密 | 基础层 + SQLite | tests/test_account_storage.cpp |
| 7 | ConfigManager | 数据层 | data/ConfigManager.h/.cpp | 配置读写、默认值、热加载 | 基础层 | tests/test_config_manager.cpp |
| 8 | MinecraftVersionService | 服务层 | service/MinecraftVersionService.h/.cpp | 版本列表拉取、缓存、查询 | 基础层 + 数据层 | tests/test_version_service.cpp |
| 9 | VersionInstaller | 服务层 | service/VersionInstaller.h/.cpp | 安装版本（client.jar、libraries、assets） | 服务层(12) + 数据层 + 基础层 | tests/test_version_installer.cpp |
| 10 | ModLoaderInstaller | 服务层 | service/ModLoaderInstaller.h/.cpp | 安装 Forge / Fabric / OptiFine / LiteLoader / Quilt | 服务层(13,12) + 基础层 | tests/test_modloader_installer.cpp |
| 11 | GameLauncher | 服务层 | service/GameLauncher.h/.cpp | 构造启动参数、启动进程、崩溃诊断 | 服务层(13) + 平台适配层 + 基础层 | tests/test_game_launcher.cpp（模拟 Java） |
| 12 | DownloadService | 服务层 | service/DownloadService.h/.cpp | 多线程分片下载、断点续传、镜像切换、限速 | 基础层 + 网络层（Qt Network） | tests/test_download_service.cpp（Mock 服务器） |
| 13 | JavaManager | 服务层 | service/JavaManager.h/.cpp | 系统 Java 检测、自动下载 JRE 17 | 服务层(12) + 平台适配层 + 基础层 | tests/test_java_manager.cpp |
| 14 | SelfUpdater | 服务层 | service/SelfUpdater.h/.cpp | GitHub Release 检查、下载更新、应用、回滚 | 服务层(12) + 基础层 | tests/test_self_updater.cpp（模拟 GitHub API） |
| 15 | MainWindow | UI 层 | ui/MainWindow.h/.cpp | 主窗口布局（标题栏 + 侧边栏 + 内容区） | 服务层接口（仅接口） | — |
| 16 | TitleBar | UI 层 | ui/TitleBar.h/.cpp | 自定义标题栏（图标、标题、窗口控制） | — | — |
| 17 | NavigationBar | UI 层 | ui/NavigationBar.h/.cpp | 侧边栏导航（账户头像、页面按钮） | — | — |
| 18 | SettingsDialog | UI 层 | ui/SettingsDialog.h/.cpp | 所有设置的显示和编辑 | 服务层接口（IConfigService） | — |
| 19 | LogViewer | UI 层 | ui/LogViewer.h/.cpp | 实时日志显示、级别过滤、搜索、导出 | 服务层（ILogger） | — |
| 20 | ApplicationBootstrapper | 入口层 | entry/ApplicationBootstrapper.h/.cpp | 参数解析、初始化、依赖注入、创建窗口、事件循环 | 所有层 | — |

### 3.1 基础层模块（第 1 层，4 个模块）

#### 模块 1：Logger（日志模块）

- **文件**：src/core/Logger.h, src/core/Logger.cpp
- **职责**：异步日志记录，多输出目标，级别过滤
- **依赖**：无
- **测试**：tests/test_logger.cpp

```cpp
class ILogger {
public:
    virtual ~ILogger() = default;
    virtual void log(LogLevel level, const std::string& file, int line, 
                     const std::string& msg, const std::map<std::string, std::string>& extra = {}) = 0;
    virtual void setConsoleOutput(bool enable) = 0;
    virtual void setMinConsoleLevel(LogLevel level) = 0;
    virtual void setLogFile(const std::string& path) = 0;
    virtual void flush() = 0;
};

class Logger : public ILogger {
    // 单例实现
};
```

**完成标准**：
- 日志文件生成于 `logs/MKL_日期.log`
- `--console` 开启控制台输出
- `MKL_FATAL` 触发弹窗显示最近 50 行

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
    static std::string getFileSize(const std::string& path);
    static std::string getFileSha1(const std::string& path);
    static std::string getTempDirectory();
    static std::string getAppDataDirectory();   // 跨平台用户目录
    static std::string joinPath(const std::vector<std::string>& parts);
};
```

#### 模块 4：StringUtils（字符串工具模块）

- **文件**：src/core/StringUtils.h, src/core/StringUtils.cpp
- **职责**：字符串处理、JSON 解析、格式化
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

### 3.2 数据层模块（第 2 层，3 个模块）

#### 模块 5：AccountData（账户数据结构）

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

#### 模块 6：AccountStorage（账户存储模块）

- **文件**：src/data/AccountStorage.h, src/data/AccountStorage.cpp
- **职责**：账户数据的 SQLite 存储、查询、加密
- **依赖**：基础层（StringUtils, FileUtils）+ SQLite
- **测试**：tests/test_account_storage.cpp

```cpp
class IAccountStorage {
public:
    virtual ~IAccountStorage() = default;
    virtual bool save(const AccountData& account) = 0;
    virtual bool remove(const std::string& id) = 0;
    virtual std::optional<AccountData> get(const std::string& id) = 0;
    virtual std::vector<AccountData> getAll() = 0;
    virtual bool setCurrent(const std::string& id) = 0;
    virtual std::optional<AccountData> getCurrent() = 0;
};

class AccountStorage : public IAccountStorage {
    // SQLite 实现
};
```

#### 模块 7：ConfigManager（配置管理模块）

- **文件**：src/data/ConfigManager.h, src/data/ConfigManager.cpp
- **职责**：启动器配置的读写、默认值、热加载
- **依赖**：基础层（FileUtils, StringUtils）
- **测试**：tests/test_config_manager.cpp

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
    std::string proxyPassword;
    std::string consoleLogLevel;
    int logRetentionDays;
};

class ConfigManager {
public:
    static ConfigManager& instance();
    AppConfig load();
    bool save(const AppConfig& config);
    void resetToDefaults();
};
```

### 3.3 服务层模块（第 3 层，7 个模块）

#### 模块 8：MinecraftVersionService（版本服务模块）

- **文件**：src/service/MinecraftVersionService.h, src/service/MinecraftVersionService.cpp
- **职责**：Minecraft 版本列表拉取、缓存、查询
- **依赖**：基础层 + 数据层
- **测试**：tests/test_version_service.cpp

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
    virtual std::vector<VersionInfo> fetchVersionList() = 0;
    virtual std::optional<VersionInfo> getVersion(const std::string& id) = 0;
    virtual void refreshCache() = 0;
};
```

#### 模块 9：VersionInstaller（版本安装模块）

- **文件**：src/service/VersionInstaller.h, src/service/VersionInstaller.cpp
- **职责**：下载并安装指定版本的 Minecraft 客户端（client.jar、libraries、assets）
- **依赖**：服务层（IDownloadService）+ 数据层 + 基础层
- **测试**：tests/test_version_installer.cpp

```cpp
struct InstallProgress {
    float percent;
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
    virtual Result<bool> install(const InstallOptions& options, 
                                  std::function<void(const InstallProgress&)> progress) = 0;
    virtual Result<bool> uninstall(const std::string& versionId) = 0;
};
```

#### 模块 10：ModLoaderInstaller（模组加载器安装模块）

- **文件**：src/service/ModLoaderInstaller.h, src/service/ModLoaderInstaller.cpp
- **职责**：安装 Forge、Fabric、OptiFine、LiteLoader、Quilt
- **依赖**：服务层（JavaManager, IDownloadService）+ 基础层
- **测试**：tests/test_modloader_installer.cpp

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
    virtual Result<bool> install(const ModLoaderInstallOptions& options,
                                  std::function<void(const std::string&)> output) = 0;
};
```

#### 模块 11：GameLauncher（游戏启动模块）

- **文件**：src/service/GameLauncher.h, src/service/GameLauncher.cpp
- **职责**：构造启动参数，启动游戏进程，捕获输出，崩溃诊断
- **依赖**：服务层（JavaManager）+ 平台适配层 + 基础层
- **测试**：tests/test_game_launcher.cpp（使用模拟 Java）

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

class IGameLauncher {
public:
    virtual ~IGameLauncher() = default;
    virtual Result<ProcessHandle> launch(const LaunchOptions& options) = 0;
    virtual void stop(ProcessHandle handle) = 0;
    virtual bool isRunning(ProcessHandle handle) = 0;
    virtual void setOutputCallback(std::function<void(const std::string&, bool isError)> callback) = 0;
};
```

#### 模块 12：DownloadService（下载服务模块）

- **文件**：src/service/DownloadService.h, src/service/DownloadService.cpp
- **职责**：多线程分片下载、断点续传、镜像切换、速度限制
- **依赖**：基础层（FileUtils, StringUtils）+ 网络层（Qt Network）
- **测试**：tests/test_download_service.cpp（Mock 服务器）

```cpp
struct DownloadTask {
    std::string url;
    std::string destPath;
    int priority;              // 0:高, 1:中, 2:低
    std::optional<int64_t> expectedSize;
    std::optional<std::string> expectedSha1;
};

struct DownloadProgress {
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
    virtual void addTask(const DownloadTask& task) = 0;
    virtual void pauseTask(const std::string& url) = 0;
    virtual void resumeTask(const std::string& url) = 0;
    virtual void cancelTask(const std::string& url) = 0;
    virtual void pauseAll() = 0;
    virtual void resumeAll() = 0;
    virtual void setOptions(const DownloadOptions& options) = 0;
    virtual std::vector<DownloadProgress> getProgress() = 0;
};
```

#### 模块 13：JavaManager（Java 管理模块）

- **文件**：src/service/JavaManager.h, src/service/JavaManager.cpp
- **职责**：系统 Java 检测、自动下载 JRE 17（含 GLIBC 兼容）
- **依赖**：服务层（IDownloadService）+ 平台适配层 + 基础层
- **测试**：tests/test_java_manager.cpp

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
    virtual bool hasJava17OrLater() = 0;
    virtual Result<std::string> getOrDownloadJava17() = 0;  // 返回 Java 路径
    virtual void setCustomJavaPath(const std::string& path) = 0;
    virtual std::string getCurrentJavaPath() = 0;
};
```

#### 模块 14：SelfUpdater（自更新模块）

- **文件**：src/service/SelfUpdater.h, src/service/SelfUpdater.cpp
- **职责**：检查 GitHub Release、下载更新、应用更新、回滚
- **依赖**：服务层（IDownloadService）+ 基础层
- **测试**：tests/test_self_updater.cpp（模拟 GitHub API）

```cpp
struct UpdateInfo {
    std::string version;
    std::string releaseUrl;
    std::map<std::string, std::string> assets;  // platform -> downloadUrl
};

class ISelfUpdater {
public:
    virtual ~ISelfUpdater() = default;
    virtual Result<UpdateInfo> checkForUpdates() = 0;
    virtual Result<bool> downloadUpdate(const UpdateInfo& info) = 0;
    virtual Result<bool> applyUpdate() = 0;
    virtual Result<bool> rollback() = 0;
};
```

### 3.4 UI 层模块（第 4 层，5 个模块）

> 原规格此处标注「6 个模块」，实际列出 5 个（模块 15–19），已更正；入口层单独为第 5 层（模块 20）。

#### 模块 15：MainWindow（主窗口模块）

- **文件**：src/ui/MainWindow.h, src/ui/MainWindow.cpp
- **职责**：主窗口布局（标题栏、侧边栏导航、内容区域）
- **依赖**：所有 UI 控件、服务层接口（只依赖接口，不依赖具体实现）

```cpp
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();
    void switchToPage(PageType page);
signals:
    void navigationChanged(PageType page);
private:
    TitleBar* m_titleBar;
    NavigationBar* m_navBar;
    QStackedWidget* m_contentStack;
    void setupUI();
};
```

#### 模块 16：TitleBar（标题栏控件）

- **文件**：src/ui/TitleBar.h, src/ui/TitleBar.cpp
- **职责**：自定义标题栏（图标、标题、最小化、最大化、关闭）

```cpp
class TitleBar : public QWidget {
    Q_OBJECT
public:
    explicit TitleBar(QWidget* parent = nullptr);
    void setWindowTitleText(const QString& title);
signals:
    void minimizeClicked();
    void maximizeClicked();
    void closeClicked();
    void doubleClicked();   // 双击全屏切换
};
```

#### 模块 17：NavigationBar（导航栏控件）

- **文件**：src/ui/NavigationBar.h, src/ui/NavigationBar.cpp
- **职责**：侧边栏导航（账户头像、页面按钮）

```cpp
class NavigationBar : public QWidget {
    Q_OBJECT
public:
    explicit NavigationBar(QWidget* parent = nullptr);
    void setCurrentPage(PageType page);
    void setAccountInfo(const QString& username, const QPixmap& avatar);
signals:
    void pageSelected(PageType page);
    void accountMenuRequested();
};
```

#### 模块 18：SettingsDialog（设置对话框）

- **文件**：src/ui/SettingsDialog.h, src/ui/SettingsDialog.cpp
- **职责**：所有设置的显示和编辑
- **依赖**：服务层接口（IConfigService）

#### 模块 19：LogViewer（日志查看器）

- **文件**：src/ui/LogViewer.h, src/ui/LogViewer.cpp
- **职责**：实时日志显示，级别过滤，搜索，导出
- **依赖**：服务层（ILogger）

### 3.5 入口层模块（第 5 层，1 个模块）

#### 模块 20：ApplicationBootstrapper（应用启动组装器）

- **文件**：src/entry/ApplicationBootstrapper.h, src/entry/ApplicationBootstrapper.cpp
- **职责**：
  - 解析命令行参数
  - 初始化日志系统
  - 创建所有服务实例
  - 注入依赖
  - 创建主窗口
  - 运行事件循环
- **依赖**：所有层所有模块

---

## 第四部分：UI 层模块化规范

### 4.1 UI 模块依赖原则

- UI 层模块只依赖服务层的**抽象接口**（不依赖具体类）
- UI 层模块之间不能互相依赖（每个控件独立）
- UI 层模块通过信号槽通信，不直接调用对方方法

### 4.2 页面枚举

```cpp
enum class PageType {
    Home,
    VersionManager,
    ModManager,
    DownloadCenter,
    LogViewer,
    Settings
};
```

### 4.3 样式规范（QSS）

```cpp
// 所有 UI 模块通过一个统一的样式管理器加载样式
class StyleManager {
public:
    static void applyStyle(QWidget* widget, const QString& theme = "light");
    static void switchTheme(const QString& theme);
};
```

---

## 第五部分：功能清单汇总

### 5.1 所有功能模块汇总

| 编号 | 模块名 | 层级 | 职责 |
| --- | --- | --- | --- |
| 1 | Logger | 基础层 | 异步日志 |
| 2 | PlatformDetector | 基础层 | 平台检测 |
| 3 | FileUtils | 基础层 | 文件操作 |
| 4 | StringUtils | 基础层 | 字符串处理 |
| 5 | AccountData | 数据层 | 账户数据结构 |
| 6 | AccountStorage | 数据层 | 账户存储 |
| 7 | ConfigManager | 数据层 | 配置管理 |
| 8 | MinecraftVersionService | 服务层 | 版本列表 |
| 9 | VersionInstaller | 服务层 | 版本安装 |
| 10 | ModLoaderInstaller | 服务层 | 模组加载器安装 |
| 11 | GameLauncher | 服务层 | 游戏启动 |
| 12 | DownloadService | 服务层 | 下载管理 |
| 13 | JavaManager | 服务层 | Java 管理 |
| 14 | SelfUpdater | 服务层 | 自更新 |
| 15 | MainWindow | UI 层 | 主窗口 |
| 16 | TitleBar | UI 层 | 标题栏 |
| 17 | NavigationBar | UI 层 | 导航栏 |
| 18 | SettingsDialog | UI 层 | 设置对话框 |
| 19 | LogViewer | UI 层 | 日志查看器 |
| 20 | ApplicationBootstrapper | 入口层 | 启动组装 |

---

## 第六部分：CI/CD 构建矩阵

| 平台 | runner | 产物 |
| --- | --- | --- |
| Windows 10/11 | windows-2025 | .exe + .zip |
| Windows 7 | self-hosted（可选） | 验证 |
| macOS 14/15 | macos-14, macos-15 | .dmg |
| Ubuntu 22.04 | ubuntu-22.04 | .AppImage + .deb |
| Ubuntu 24.04 | ubuntu-24.04 | .AppImage |
| Fedora 39 | self-hosted | .rpm |
| Android | ubuntu-latest | .apk |
| iOS | macos-14 | 未签名 .app |

---

## 第七部分：交付物清单

1. 启动器完整源码（所有模块独立文件，按目录组织）
2. TouchUI Mod 源码（独立 JAR 项目）
3. 官网完整源码（Next.js）
4. GitHub Actions 工作流（build.yml, test-compatibility.yml, deploy.yml）
5. 文档（README.md, user_manual.md, developer_guide.md, plugin_development.md, ios_sideload.md）
6. 资源文件（图标集、样式表、默认布局 JSON、键盘贴图）
7. 示例插件（空插件源码 + .mklplugin 打包示例）

> **总要求**：严格按照以上规格生成完整的项目代码、配置文件和文档。所有模块必须独立、清晰、可测试。

---

## 附录 A：整理修正记录

本次整理对原规格做的改动（仅修正笔误与格式，未变更任何规格内容）：

1. **模块总数**：第三部分标题「19 个独立模块」→ **20 个独立模块**（实际编号 1–20，与 5.1 清单一致）。
2. **UI 层模块数**：3.4 节「6 个模块」→ **5 个模块**（实际列出模块 15–19；模块 20 属于入口层第 5 层）。
3. **新增 3.0 模块总览索引**：将原 5.1 清单升级为含文件路径、依赖、测试文件的总览表，便于快速定位。
4. **格式统一**：全部小节统一为「文件 / 职责 / 依赖 / 测试 / 接口 / 完成标准」结构；列表符号统一；代码块统一为 C++ 高亮。
5. **补充目录锚点**：增加目录、附录，方便导航与评审。

## 附录 B：规格中待补充定义的类型

以下类型在规格中被引用但未定义所在模块，建议在开发阶段于基础层（或平台抽象层）补充，不影响本规格的模块划分：

| 类型 | 引用位置 | 建议归属 |
| --- | --- | --- |
| `Result<T>` / `Error` / `ErrorCode` | 2.5 错误处理模板、模块 9/10/11/13/14 | 基础层（core/） |
| `LogLevel` | 模块 1 Logger | 基础层（core/Logger.h） |
| `ProcessHandle` | 模块 11 GameLauncher | 基础层或平台抽象层（platform/） |
| `IConfigService` | 模块 18 SettingsDialog | 服务层（对 ConfigManager 的接口抽象） |
| 网络层 | 模块 12 依赖「Qt Network」 | 服务层内部直接使用 Qt Network 提供 |
