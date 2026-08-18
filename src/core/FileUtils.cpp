#include "FileUtils.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>

namespace fs = std::filesystem;

namespace mkl {

bool FileUtils::exists(const std::string& path) {
    std::error_code ec;
    return fs::exists(path, ec) && !ec;
}

bool FileUtils::createDirectories(const std::string& path) {
    std::error_code ec;
    fs::create_directories(path, ec);
    return !ec;
}

std::string FileUtils::readAllText(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        return "";
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

bool FileUtils::writeAllText(const std::string& path, const std::string& content) {
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) {
        return false;
    }
    f.write(content.data(), static_cast<std::streamsize>(content.size()));
    return f.good();
}

bool FileUtils::copyFile(const std::string& src, const std::string& dest) {
    std::error_code ec;
    fs::copy_file(src, dest, fs::copy_options::overwrite_existing, ec);
    return !ec;
}

bool FileUtils::moveFile(const std::string& src, const std::string& dest) {
    std::error_code ec;
    fs::rename(src, dest, ec);
    if (ec) {
        // 跨文件系统时 rename 可能失败，退化为 copy + delete
        if (copyFile(src, dest)) {
            return deleteFile(src);
        }
        return false;
    }
    return true;
}

bool FileUtils::deleteFile(const std::string& path) {
    std::error_code ec;
    fs::remove(path, ec);
    return !ec;
}

bool FileUtils::deleteDirectory(const std::string& path) {
    std::error_code ec;
    fs::remove_all(path, ec);
    return !ec;
}

uint64_t FileUtils::getFileSize(const std::string& path) {
    std::error_code ec;
    auto size = fs::file_size(path, ec);
    return ec ? 0 : static_cast<uint64_t>(size);
}

std::string FileUtils::formatFileSize(uint64_t bytes) {
    static const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    double value = static_cast<double>(bytes);
    int unit = 0;
    while (value >= 1024.0 && unit < 4) {
        value /= 1024.0;
        ++unit;
    }
    std::ostringstream os;
    if (unit == 0) {
        os << bytes << " B";
    } else {
        os << std::fixed << std::setprecision(1) << value << " " << units[unit];
    }
    return os.str();
}

// ---- SHA-1（C++17 标准库，无第三方依赖） ----

namespace {

inline uint32_t rol32(uint32_t x, int n) {
    return (x << n) | (x >> (32 - n));
}

std::string sha1Hex(const std::vector<uint8_t>& data) {
    uint64_t bitlen = static_cast<uint64_t>(data.size()) * 8ULL;

    std::vector<uint8_t> msg = data;
    msg.push_back(0x80);
    while (msg.size() % 64 != 56) {
        msg.push_back(0x00);
    }
    for (int i = 7; i >= 0; --i) {
        msg.push_back(static_cast<uint8_t>((bitlen >> (i * 8)) & 0xFF));
    }

    uint32_t h0 = 0x67452301, h1 = 0xEFCDAB89, h2 = 0x98BADCFE, h3 = 0x10325476,
             h4 = 0xC3D2E1F0;

    for (size_t i = 0; i + 64 <= msg.size(); i += 64) {
        uint32_t w[80];
        for (int j = 0; j < 16; ++j) {
            w[j] = (static_cast<uint32_t>(msg[i + j * 4]) << 24) |
                   (static_cast<uint32_t>(msg[i + j * 4 + 1]) << 16) |
                   (static_cast<uint32_t>(msg[i + j * 4 + 2]) << 8) |
                   static_cast<uint32_t>(msg[i + j * 4 + 3]);
        }
        for (int j = 16; j < 80; ++j) {
            w[j] = rol32(w[j - 3] ^ w[j - 8] ^ w[j - 14] ^ w[j - 16], 1);
        }
        uint32_t a = h0, b = h1, c = h2, d = h3, e = h4;
        for (int j = 0; j < 80; ++j) {
            uint32_t f, k;
            if (j < 20) {
                f = (b & c) | (~b & d);
                k = 0x5A827999;
            } else if (j < 40) {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1;
            } else if (j < 60) {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDC;
            } else {
                f = b ^ c ^ d;
                k = 0xCA62C1D6;
            }
            uint32_t temp = rol32(a, 5) + f + e + k + w[j];
            e = d;
            d = c;
            c = rol32(b, 30);
            b = a;
            a = temp;
        }
        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
        h4 += e;
    }

    std::ostringstream os;
    os << std::hex << std::setfill('0');
    const uint32_t hs[5] = {h0, h1, h2, h3, h4};
    for (uint32_t h : hs) {
        os << std::setw(8) << h;
    }
    return os.str();
}

} // namespace

std::string FileUtils::getFileSha1(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        return "";
    }
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(f)),
                              std::istreambuf_iterator<char>());
    return sha1Hex(data);
}

std::string FileUtils::getTempDirectory() {
    std::error_code ec;
    auto p = fs::temp_directory_path(ec);
    return ec ? std::string("/tmp") : p.string();
}

std::string FileUtils::getAppDataDirectory() {
#if defined(_WIN32)
    const char* appdata = std::getenv("APPDATA");
    if (appdata && *appdata) {
        return appdata;
    }
#elif defined(__APPLE__)
    const char* home = std::getenv("HOME");
    if (home && *home) {
        return std::string(home) + "/Library/Application Support";
    }
#else
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    if (xdg && *xdg) {
        return xdg;
    }
    const char* home = std::getenv("HOME");
    if (home && *home) {
        return std::string(home) + "/.config";
    }
#endif
    return getTempDirectory();
}

std::string FileUtils::joinPath(const std::vector<std::string>& parts) {
    if (parts.empty()) {
        return "";
    }
    fs::path p(parts[0]);
    for (size_t i = 1; i < parts.size(); ++i) {
        p /= parts[i];
    }
    return p.string();
}

} // namespace mkl
