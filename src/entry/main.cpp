#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QTimer>
#include <QUrl>

#include "FileUtils.h"
#include "Logger.h"
#include "PlatformDetector.h"

// MKL 入口（规格：模块 31 ApplicationBootstrapper 的最小实现）
// 职责：初始化日志 → 平台检测 → 加载 QML 主窗口（空白窗口骨架）→ 事件循环
int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("MKL-Team"));
    QCoreApplication::setApplicationName(QStringLiteral("MKL"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.0.1-alpha"));

    // ---- 基础层：日志初始化 ----
    auto& logger = mkl::Logger::instance();
    logger.setConsoleOutput(true);
    const std::string logsDir = mkl::FileUtils::getAppDataDirectory() + "/mkl/logs";
    mkl::FileUtils::createDirectories(logsDir);
    logger.setLogFile(logsDir + "/MKL.log");
    MKL_INFO("MKL v0.0.1-alpha 启动");

    // ---- 基础层：平台检测 ----
    const mkl::PlatformInfo info = mkl::PlatformDetector::detect();
    std::string osName;
    switch (info.os) {
        case mkl::OSType::Windows: osName = "Windows"; break;
        case mkl::OSType::macOS: osName = "macOS"; break;
        case mkl::OSType::Linux: osName = "Linux"; break;
        case mkl::OSType::Android: osName = "Android"; break;
        case mkl::OSType::iOS: osName = "iOS"; break;
        default: osName = "Unknown"; break;
    }
    MKL_INFO("平台: " + osName + " 版本=" + info.osVersion + " 架构=" + info.arch +
             (info.glibcVersion.empty() ? "" : " glibc=" + info.glibcVersion));

    // ---- UI 层：加载 QML 主窗口（空白窗口骨架） ----
    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/qml/MainWindow.qml"));

    // --smoke-test：无头冒烟测试（CI 自动化测试用）
    const bool smoke = app.arguments().contains(QStringLiteral("--smoke-test"));
    if (smoke) {
        QObject::connect(&engine, &QQmlApplicationEngine::objectCreated, &app,
                         [&app](QObject* obj, const QUrl&) {
                             if (obj) {
                                 QTimer::singleShot(500, &app, &QCoreApplication::quit);
                             } else {
                                 QCoreApplication::exit(2);
                             }
                         });
        QTimer::singleShot(15000, &app, [] { QCoreApplication::exit(3); }); // 超时兜底
    }

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

    engine.load(url);

    if (!smoke) {
        MKL_INFO("主窗口已加载，进入事件循环");
    }
    const int rc = app.exec();
    MKL_INFO(std::string("MKL 退出，返回码 ") + std::to_string(rc));
    logger.flush();
    return rc;
}
