#include "StringUtils.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace mkl {

std::string StringUtils::trim(const std::string& s) {
    size_t start = 0;
    size_t end = s.size();
    while (start < end && std::isspace(static_cast<unsigned char>(s[start]))) {
        ++start;
    }
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
        --end;
    }
    return s.substr(start, end - start);
}

std::vector<std::string> StringUtils::split(const std::string& s, char delimiter) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
        if (c == delimiter) {
            out.push_back(cur);
            cur.clear();
        } else {
            cur.push_back(c);
        }
    }
    out.push_back(cur);
    return out;
}

std::string StringUtils::replace(const std::string& s, const std::string& from,
                                 const std::string& to) {
    if (from.empty()) {
        return s;
    }
    std::string out;
    size_t pos = 0;
    while (true) {
        size_t found = s.find(from, pos);
        if (found == std::string::npos) {
            out.append(s, pos, std::string::npos);
            break;
        }
        out.append(s, pos, found - pos);
        out.append(to);
        pos = found + from.size();
    }
    return out;
}

bool StringUtils::startsWith(const std::string& s, const std::string& prefix) {
    return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

bool StringUtils::endsWith(const std::string& s, const std::string& suffix) {
    return s.size() >= suffix.size() &&
           s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string StringUtils::toLower(const std::string& s) {
    std::string out = s;
    for (char& c : out) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return out;
}

std::string StringUtils::toUpper(const std::string& s) {
    std::string out = s;
    for (char& c : out) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return out;
}

std::string StringUtils::format(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    va_list copy;
    va_copy(copy, args);
    int needed = std::vsnprintf(nullptr, 0, fmt, copy);
    va_end(copy);
    if (needed < 0) {
        va_end(args);
        return "";
    }
    std::string out(static_cast<size_t>(needed), '\0');
    std::vsnprintf(&out[0], static_cast<size_t>(needed) + 1, fmt, args);
    va_end(args);
    return out;
}

std::map<std::string, std::string> StringUtils::parseQueryString(const std::string& qs) {
    std::map<std::string, std::string> out;
    std::string body = qs;
    if (!body.empty() && body[0] == '?') {
        body = body.substr(1);
    }
    for (const auto& pair : split(body, '&')) {
        if (pair.empty()) {
            continue;
        }
        size_t eq = pair.find('=');
        if (eq == std::string::npos) {
            out[pair] = "";
        } else {
            out[pair.substr(0, eq)] = pair.substr(eq + 1);
        }
    }
    return out;
}

} // namespace mkl
