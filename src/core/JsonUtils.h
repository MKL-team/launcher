#pragma once

#include <map>
#include <string>
#include <vector>

namespace mkl {

// JSON 工具（规格：模块 5 JsonUtils）
// 零依赖：内置轻量 JSON 解析器，支持对象/数组/字符串/数字/布尔/null 与点号路径查询
class JsonUtils {
public:
    static bool isJson(const std::string& s);

    // keyPath 用点号分隔，如 "version.id"、"arguments.game.0.value"
    static std::string getString(const std::string& json, const std::string& keyPath,
                                 const std::string& def = "");
    static int64_t getInt(const std::string& json, const std::string& keyPath, int64_t def = 0);
    static bool getBool(const std::string& json, const std::string& keyPath, bool def = false);
    static std::vector<std::string> getStringArray(const std::string& json,
                                                   const std::string& keyPath);
    static std::map<std::string, std::string> getStringMap(const std::string& json,
                                                           const std::string& keyPath);

    static std::string serializeObject(const std::map<std::string, std::string>& obj);
    static std::string serializeStringArray(const std::vector<std::string>& arr);
};

} // namespace mkl
