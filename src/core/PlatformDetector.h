#pragma once

#include <string>

namespace mkl {

// 操作系统类型（规格：模块 2 PlatformDetector）
enum class OSType {
    Windows,
    macOS,
    Linux,
    Android,
    iOS,
    Unknown
};

struct PlatformInfo {
    OSType os = OSType::Unknown;
    std::string osVersion;
    std::string arch;          // "x64", "arm64"
    std::string glibcVersion;  // Linux only
    int androidApiLevel = 0;   // Android only
};

class PlatformDetector {
public:
    static PlatformInfo detect();
    static bool isWindows7();
    static bool isMacOS14OrLater();
    static bool isMobile();     // Android || iOS
};

} // namespace mkl
