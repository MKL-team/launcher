#include "JsonUtils.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <sstream>

namespace mkl {

namespace detail {

// 轻量 JSON 节点树
struct JNode {
    enum Type { Null, Bool, Number, String, Array, Object } type = Null;
    bool b = false;
    double num = 0;
    std::string str;
    std::vector<JNode> arr;
    std::vector<std::pair<std::string, JNode>> obj;
};

class Parser {
public:
    explicit Parser(const std::string& s) : m_s(s) {}

    bool parse() {
        skipWs();
        if (!parseValue(m_root)) {
            return false;
        }
        skipWs();
        return m_pos == m_s.size();
    }

    const JNode& root() const { return m_root; }

private:
    const std::string& m_s;
    size_t m_pos = 0;
    JNode m_root;

    void skipWs() {
        while (m_pos < m_s.size() && std::isspace(static_cast<unsigned char>(m_s[m_pos]))) {
            ++m_pos;
        }
    }

    bool parseValue(JNode& out) {
        if (m_pos >= m_s.size()) {
            return false;
        }
        char c = m_s[m_pos];
        if (c == '{') {
            return parseObject(out);
        }
        if (c == '[') {
            return parseArray(out);
        }
        if (c == '"') {
            out.type = JNode::String;
            return parseString(out.str);
        }
        if (c == 't') {
            return parseLiteral("true", out, JNode::Bool, true);
        }
        if (c == 'f') {
            return parseLiteral("false", out, JNode::Bool, false);
        }
        if (c == 'n') {
            return parseLiteral("null", out, JNode::Null, false);
        }
        return parseNumber(out);
    }

    bool parseLiteral(const char* lit, JNode& out, JNode::Type type, bool boolValue) {
        size_t len = std::strlen(lit);
        if (m_s.compare(m_pos, len, lit) != 0) {
            return false;
        }
        m_pos += len;
        out.type = type;
        out.b = boolValue;
        return true;
    }

    static void appendUtf8(std::string& s, unsigned int cp) {
        if (cp < 0x80) {
            s.push_back(static_cast<char>(cp));
        } else if (cp < 0x800) {
            s.push_back(static_cast<char>(0xC0 | (cp >> 6)));
            s.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp < 0x10000) {
            s.push_back(static_cast<char>(0xE0 | (cp >> 12)));
            s.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            s.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            s.push_back(static_cast<char>(0xF0 | (cp >> 18)));
            s.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            s.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            s.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
    }

    bool parseString(std::string& out) {
        if (m_s[m_pos] != '"') {
            return false;
        }
        ++m_pos;
        out.clear();
        while (m_pos < m_s.size()) {
            char c = m_s[m_pos++];
            if (c == '"') {
                return true;
            }
            if (c != '\\') {
                out.push_back(c);
                continue;
            }
            if (m_pos >= m_s.size()) {
                return false;
            }
            char e = m_s[m_pos++];
            switch (e) {
                case '"': out.push_back('"'); break;
                case '\\': out.push_back('\\'); break;
                case '/': out.push_back('/'); break;
                case 'b': out.push_back('\b'); break;
                case 'f': out.push_back('\f'); break;
                case 'n': out.push_back('\n'); break;
                case 'r': out.push_back('\r'); break;
                case 't': out.push_back('\t'); break;
                case 'u': {
                    if (m_pos + 4 > m_s.size()) {
                        return false;
                    }
                    unsigned int cp = 0;
                    for (int k = 0; k < 4; ++k) {
                        char h = m_s[m_pos++];
                        cp <<= 4;
                        if (h >= '0' && h <= '9') {
                            cp |= static_cast<unsigned int>(h - '0');
                        } else if (h >= 'a' && h <= 'f') {
                            cp |= static_cast<unsigned int>(h - 'a' + 10);
                        } else if (h >= 'A' && h <= 'F') {
                            cp |= static_cast<unsigned int>(h - 'A' + 10);
                        } else {
                            return false;
                        }
                    }
                    appendUtf8(out, cp);
                    break;
                }
                default: return false;
            }
        }
        return false; // 未闭合字符串
    }

    bool parseNumber(JNode& out) {
        size_t start = m_pos;
        if (m_pos < m_s.size() && m_s[m_pos] == '-') {
            ++m_pos;
        }
        bool hasDigit = false;
        while (m_pos < m_s.size() && std::isdigit(static_cast<unsigned char>(m_s[m_pos]))) {
            ++m_pos;
            hasDigit = true;
        }
        if (m_pos < m_s.size() && m_s[m_pos] == '.') {
            ++m_pos;
            while (m_pos < m_s.size() && std::isdigit(static_cast<unsigned char>(m_s[m_pos]))) {
                ++m_pos;
                hasDigit = true;
            }
        }
        if (m_pos < m_s.size() && (m_s[m_pos] == 'e' || m_s[m_pos] == 'E')) {
            ++m_pos;
            if (m_pos < m_s.size() && (m_s[m_pos] == '+' || m_s[m_pos] == '-')) {
                ++m_pos;
            }
            while (m_pos < m_s.size() && std::isdigit(static_cast<unsigned char>(m_s[m_pos]))) {
                ++m_pos;
            }
        }
        if (!hasDigit) {
            return false;
        }
        out.type = JNode::Number;
        out.num = std::strtod(m_s.substr(start, m_pos - start).c_str(), nullptr);
        return true;
    }

    bool parseObject(JNode& out) {
        ++m_pos; // '{'
        out.type = JNode::Object;
        skipWs();
        if (m_pos < m_s.size() && m_s[m_pos] == '}') {
            ++m_pos;
            return true;
        }
        while (true) {
            skipWs();
            std::string key;
            if (!parseString(key)) {
                return false;
            }
            skipWs();
            if (m_pos >= m_s.size() || m_s[m_pos] != ':') {
                return false;
            }
            ++m_pos;
            skipWs();
            JNode v;
            if (!parseValue(v)) {
                return false;
            }
            out.obj.emplace_back(std::move(key), std::move(v));
            skipWs();
            if (m_pos >= m_s.size()) {
                return false;
            }
            if (m_s[m_pos] == ',') {
                ++m_pos;
                continue;
            }
            if (m_s[m_pos] == '}') {
                ++m_pos;
                return true;
            }
            return false;
        }
    }

    bool parseArray(JNode& out) {
        ++m_pos; // '['
        out.type = JNode::Array;
        skipWs();
        if (m_pos < m_s.size() && m_s[m_pos] == ']') {
            ++m_pos;
            return true;
        }
        while (true) {
            skipWs();
            JNode v;
            if (!parseValue(v)) {
                return false;
            }
            out.arr.push_back(std::move(v));
            skipWs();
            if (m_pos >= m_s.size()) {
                return false;
            }
            if (m_s[m_pos] == ',') {
                ++m_pos;
                continue;
            }
            if (m_s[m_pos] == ']') {
                ++m_pos;
                return true;
            }
            return false;
        }
    }
};

std::vector<std::string> splitPath(const std::string& keyPath) {
    std::vector<std::string> segs;
    std::string cur;
    for (char c : keyPath) {
        if (c == '.') {
            if (!cur.empty()) {
                segs.push_back(cur);
            }
            cur.clear();
        } else {
            cur.push_back(c);
        }
    }
    if (!cur.empty()) {
        segs.push_back(cur);
    }
    return segs;
}

const JNode* findPath(const JNode& node, const std::vector<std::string>& segs) {
    const JNode* cur = &node;
    for (const auto& seg : segs) {
        if (!cur) {
            return nullptr;
        }
        if (cur->type == JNode::Object) {
            const JNode* next = nullptr;
            for (const auto& kv : cur->obj) {
                if (kv.first == seg) {
                    next = &kv.second;
                    break;
                }
            }
            cur = next;
        } else if (cur->type == JNode::Array) {
            bool isNum = !seg.empty() &&
                         std::all_of(seg.begin(), seg.end(), [](char c) {
                             return std::isdigit(static_cast<unsigned char>(c));
                         });
            if (!isNum) {
                return nullptr;
            }
            size_t idx = static_cast<size_t>(std::strtoul(seg.c_str(), nullptr, 10));
            if (idx >= cur->arr.size()) {
                return nullptr;
            }
            cur = &cur->arr[idx];
        } else {
            return nullptr;
        }
    }
    return cur;
}

std::string escapeString(const std::string& s) {
    std::string out;
    out.push_back('"');
    for (char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out.push_back(c);
        }
    }
    out.push_back('"');
    return out;
}

} // namespace detail

bool JsonUtils::isJson(const std::string& s) {
    detail::Parser p(s);
    return p.parse();
}

std::string JsonUtils::getString(const std::string& json, const std::string& keyPath,
                                 const std::string& def) {
    detail::Parser p(json);
    if (!p.parse()) {
        return def;
    }
    const auto* n = detail::findPath(p.root(), detail::splitPath(keyPath));
    if (n && n->type == detail::JNode::String) {
        return n->str;
    }
    return def;
}

int64_t JsonUtils::getInt(const std::string& json, const std::string& keyPath, int64_t def) {
    detail::Parser p(json);
    if (!p.parse()) {
        return def;
    }
    const auto* n = detail::findPath(p.root(), detail::splitPath(keyPath));
    if (!n) {
        return def;
    }
    if (n->type == detail::JNode::Number) {
        return static_cast<int64_t>(n->num);
    }
    if (n->type == detail::JNode::String) {
        try {
            return std::stoll(n->str);
        } catch (...) {
            return def;
        }
    }
    return def;
}

bool JsonUtils::getBool(const std::string& json, const std::string& keyPath, bool def) {
    detail::Parser p(json);
    if (!p.parse()) {
        return def;
    }
    const auto* n = detail::findPath(p.root(), detail::splitPath(keyPath));
    if (n && n->type == detail::JNode::Bool) {
        return n->b;
    }
    return def;
}

std::vector<std::string> JsonUtils::getStringArray(const std::string& json,
                                                   const std::string& keyPath) {
    std::vector<std::string> out;
    detail::Parser p(json);
    if (!p.parse()) {
        return out;
    }
    const auto* n = detail::findPath(p.root(), detail::splitPath(keyPath));
    if (!n || n->type != detail::JNode::Array) {
        return out;
    }
    for (const auto& item : n->arr) {
        if (item.type == detail::JNode::String) {
            out.push_back(item.str);
        }
    }
    return out;
}

std::map<std::string, std::string> JsonUtils::getStringMap(const std::string& json,
                                                           const std::string& keyPath) {
    std::map<std::string, std::string> out;
    detail::Parser p(json);
    if (!p.parse()) {
        return out;
    }
    const auto* n = detail::findPath(p.root(), detail::splitPath(keyPath));
    if (!n || n->type != detail::JNode::Object) {
        return out;
    }
    for (const auto& kv : n->obj) {
        if (kv.second.type == detail::JNode::String) {
            out[kv.first] = kv.second.str;
        }
    }
    return out;
}

std::string JsonUtils::serializeObject(const std::map<std::string, std::string>& obj) {
    std::string out = "{";
    bool first = true;
    for (const auto& kv : obj) {
        if (!first) {
            out += ",";
        }
        first = false;
        out += detail::escapeString(kv.first) + ":" + detail::escapeString(kv.second);
    }
    out += "}";
    return out;
}

std::string JsonUtils::serializeStringArray(const std::vector<std::string>& arr) {
    std::string out = "[";
    bool first = true;
    for (const auto& s : arr) {
        if (!first) {
            out += ",";
        }
        first = false;
        out += detail::escapeString(s);
    }
    out += "]";
    return out;
}

} // namespace mkl
