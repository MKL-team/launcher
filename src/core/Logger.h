#pragma once

#include <functional>
#include <map>
#include <string>
#include <vector>

#include "LogLevel.h"

namespace mkl {

// 日志接口（规格：模块 1 Logger）
class ILogger {
public:
    virtual ~ILogger() = default;
    virtual void log(LogLevel level, const std::string& file, int line,
                     const std::string& msg,
                     const std::map<std::string, std::string>& extra = {}) = 0;
    virtual void setConsoleOutput(bool enable) = 0;
    virtual void setMinConsoleLevel(LogLevel level) = 0;
    virtual void setLogFile(const std::string& path) = 0;
    virtual void flush() = 0;
    // 实时订阅（LogViewer 使用）
    virtual void subscribe(std::function<void(LogLevel, const std::string&)> sink) = 0;
    // 最近 N 行（FATAL 弹窗需要最近 50 行）
    virtual std::vector<std::string> recentLines(int count) = 0;
};

// 异步日志，多输出目标，级别过滤
// 单例提供全局默认；各模块可经构造函数注入 ILogger* 以便测试替换
class Logger : public ILogger {
public:
    static Logger& instance();

    void log(LogLevel level, const std::string& file, int line,
             const std::string& msg,
             const std::map<std::string, std::string>& extra = {}) override;
    void setConsoleOutput(bool enable) override;
    void setMinConsoleLevel(LogLevel level) override;
    void setLogFile(const std::string& path) override;
    void flush() override;
    void subscribe(std::function<void(LogLevel, const std::string&)> sink) override;
    std::vector<std::string> recentLines(int count) override;

    // FATAL 弹窗处理器（由 PlatformUI 注入；默认仅输出到控制台/文件）
    void setFatalDialogHandler(std::function<void(const std::vector<std::string>& last50)> handler);

private:
    Logger();
    ~Logger() override;
    struct Impl;
    Impl* m_impl;
};

} // namespace mkl

// 日志宏（自动携带文件/行号）
#define MKL_LOG(level, msg) ::mkl::Logger::instance().log(level, __FILE__, __LINE__, msg)
#define MKL_INFO(msg)  MKL_LOG(::mkl::LogLevel::INFO,  msg)
#define MKL_WARN(msg)  MKL_LOG(::mkl::LogLevel::WARN,  msg)
#define MKL_ERROR(msg) MKL_LOG(::mkl::LogLevel::ERROR, msg)
#define MKL_FATAL(msg) MKL_LOG(::mkl::LogLevel::FATAL, msg)
