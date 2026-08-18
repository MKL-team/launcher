#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace mkl {

// 文件工具（规格：模块 3 FileUtils）
// 仅 C++ 标准库 + 平台系统调用
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
    static uint64_t getFileSize(const std::string& path);
    static std::string formatFileSize(uint64_t bytes);
    static std::string getFileSha1(const std::string& path);
    static std::string getTempDirectory();
    static std::string getAppDataDirectory();   // 跨平台用户目录
    static std::string joinPath(const std::vector<std::string>& parts);
};

} // namespace mkl
