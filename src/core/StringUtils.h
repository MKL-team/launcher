#pragma once

#include <map>
#include <string>
#include <vector>

namespace mkl {

// 字符串工具（规格：模块 4 StringUtils；JSON 职责见 JsonUtils）
class StringUtils {
public:
    static std::string trim(const std::string& s);
    static std::vector<std::string> split(const std::string& s, char delimiter);
    static std::string replace(const std::string& s, const std::string& from,
                               const std::string& to);
    static bool startsWith(const std::string& s, const std::string& prefix);
    static bool endsWith(const std::string& s, const std::string& suffix);
    static std::string toLower(const std::string& s);
    static std::string toUpper(const std::string& s);
    static std::string format(const char* fmt, ...);
    static std::map<std::string, std::string> parseQueryString(const std::string& qs);
};

} // namespace mkl
