#include "Logger.h"

#include <chrono>
#include <ctime>
#include <deque>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>

namespace mkl {

struct Logger::Impl {
    std::mutex mutex;
    bool console = false;
    LogLevel minConsole = LogLevel::INFO;
    std::ofstream file;
    std::vector<std::function<void(LogLevel, const std::string&)>> sinks;
    std::deque<std::string> ring; // 最近 50 行
    std::function<void(const std::vector<std::string>&)> fatalHandler;
};

Logger::Logger() : m_impl(new Impl) {}

Logger::~Logger() {
    flush();
    delete m_impl;
}

Logger& Logger::instance() {
    static Logger s_instance;
    return s_instance;
}

static std::string nowString() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return buf;
}

static std::string formatLine(LogLevel level, const std::string& file, int line,
                              const std::string& msg) {
    std::ostringstream os;
    os << "[" << nowString() << "] [" << logLevelName(level) << "] " << file << ":" << line
       << " " << msg;
    return os.str();
}

void Logger::log(LogLevel level, const std::string& file, int line, const std::string& msg,
                 const std::map<std::string, std::string>& extra) {
    std::string text = formatLine(level, file, line, msg);
    for (const auto& kv : extra) {
        text += " " + kv.first + "=" + kv.second;
    }

    std::vector<std::function<void(LogLevel, const std::string&)>> sinks;
    std::vector<std::string> ringCopy;
    std::function<void(const std::vector<std::string>&)> fatal;
    const bool isFatal = (level == LogLevel::FATAL);

    {
        std::lock_guard<std::mutex> lock(m_impl->mutex);
        if (m_impl->file.is_open()) {
            m_impl->file << text << std::endl;
        }
        if (m_impl->console && level >= m_impl->minConsole) {
            std::cout << text << std::endl;
        }
        m_impl->ring.push_back(text);
        if (m_impl->ring.size() > 50) {
            m_impl->ring.pop_front();
        }
        sinks = m_impl->sinks;
        fatal = m_impl->fatalHandler;
        if (isFatal) {
            ringCopy.assign(m_impl->ring.begin(), m_impl->ring.end());
        }
    }

    for (const auto& sink : sinks) {
        sink(level, text);
    }
    if (isFatal && fatal) {
        fatal(ringCopy); // 弹窗处理器在锁外调用，避免死锁
    }
}

void Logger::setConsoleOutput(bool enable) {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->console = enable;
}

void Logger::setMinConsoleLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->minConsole = level;
}

void Logger::setLogFile(const std::string& path) {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    if (m_impl->file.is_open()) {
        m_impl->file.close();
    }
    m_impl->file.open(path, std::ios::out | std::ios::app);
}

void Logger::flush() {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    if (m_impl->file.is_open()) {
        m_impl->file.flush();
    }
    std::cout.flush();
}

void Logger::subscribe(std::function<void(LogLevel, const std::string&)> sink) {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->sinks.push_back(std::move(sink));
}

std::vector<std::string> Logger::recentLines(int count) {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    std::vector<std::string> out;
    int n = static_cast<int>(m_impl->ring.size());
    int start = n > count ? n - count : 0;
    for (int i = start; i < n; ++i) {
        out.push_back(m_impl->ring[static_cast<size_t>(i)]);
    }
    return out;
}

void Logger::setFatalDialogHandler(
    std::function<void(const std::vector<std::string>& last50)> handler) {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->fatalHandler = std::move(handler);
}

} // namespace mkl
