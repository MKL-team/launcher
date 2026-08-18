#include "FileUtils.h"
#include "Logger.h"

#include "test_util.h"

MKL_TEST(logger_writes_file) {
    auto& logger = mkl::Logger::instance();
    logger.setConsoleOutput(false);

    const std::string dir = mkl::FileUtils::getTempDirectory() + "/mkl_test_logger";
    mkl::FileUtils::deleteDirectory(dir);
    mkl::FileUtils::createDirectories(dir);
    const std::string path = dir + "/test.log";

    logger.setLogFile(path);
    logger.log(mkl::LogLevel::INFO, "test_file.cpp", 12, "hello logger");
    logger.flush();

    CHECK(mkl::FileUtils::exists(path));
    const std::string content = mkl::FileUtils::readAllText(path);
    CHECK(content.find("hello logger") != std::string::npos);
    CHECK(content.find("INFO") != std::string::npos);

    mkl::FileUtils::deleteDirectory(dir);
    return true;
}

MKL_TEST(logger_subscribe_receives) {
    auto& logger = mkl::Logger::instance();
    int received = 0;
    logger.subscribe([&received](mkl::LogLevel, const std::string&) { ++received; });
    logger.log(mkl::LogLevel::DEBUG, "t", 1, "subscribe check");
    CHECK(received >= 1);
    return true;
}

MKL_TEST(logger_ring_buffer_last50) {
    auto& logger = mkl::Logger::instance();
    for (int i = 0; i < 60; ++i) {
        logger.log(mkl::LogLevel::DEBUG, "ring", i, "line " + std::to_string(i));
    }
    const auto lines = logger.recentLines(50);
    CHECK_EQ(static_cast<int>(lines.size()), 50);
    // 60 行中保留最后 50 行，即第 10..59 行
    CHECK(lines.front().find("line 10") != std::string::npos);
    CHECK(lines.back().find("line 59") != std::string::npos);
    return true;
}

MKL_TEST(logger_extra_fields) {
    auto& logger = mkl::Logger::instance();
    std::map<std::string, std::string> extra{{"version", "0.0.1"}};
    std::string captured;
    logger.subscribe([&captured](mkl::LogLevel, const std::string& line) { captured = line; });
    logger.log(mkl::LogLevel::INFO, "extra", 1, "with extra", extra);
    CHECK(captured.find("version=0.0.1") != std::string::npos);
    return true;
}
