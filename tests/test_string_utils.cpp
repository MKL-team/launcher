#include "StringUtils.h"

#include <map>
#include <string>
#include <vector>

#include "test_util.h"

MKL_TEST(string_trim) {
    CHECK_EQ(mkl::StringUtils::trim("  hello  "), std::string("hello"));
    CHECK_EQ(mkl::StringUtils::trim("\t\n hello \n"), std::string("hello"));
    CHECK_EQ(mkl::StringUtils::trim(""), std::string(""));
    return true;
}

MKL_TEST(string_split) {
    const auto parts = mkl::StringUtils::split("a,b,c", ',');
    CHECK_EQ(parts.size(), 3U);
    CHECK_EQ(parts[0], std::string("a"));
    CHECK_EQ(parts[2], std::string("c"));
    return true;
}

MKL_TEST(string_replace) {
    CHECK_EQ(mkl::StringUtils::replace("hello world world", "world", "mkl"),
             std::string("hello mkl mkl"));
    CHECK_EQ(mkl::StringUtils::replace("abc", "x", "y"), std::string("abc"));
    return true;
}

MKL_TEST(string_prefix_suffix) {
    CHECK(mkl::StringUtils::startsWith("launcher.exe", "launcher"));
    CHECK(!mkl::StringUtils::startsWith("launcher.exe", "x"));
    CHECK(mkl::StringUtils::endsWith("launcher.exe", ".exe"));
    CHECK(!mkl::StringUtils::endsWith("launcher.exe", ".zip"));
    return true;
}

MKL_TEST(string_case) {
    CHECK_EQ(mkl::StringUtils::toLower("AbC"), std::string("abc"));
    CHECK_EQ(mkl::StringUtils::toUpper("aBc"), std::string("ABC"));
    return true;
}

MKL_TEST(string_format) {
    CHECK_EQ(mkl::StringUtils::format("%s-%d", "v", 7), std::string("v-7"));
    return true;
}

MKL_TEST(string_parse_query_string) {
    const auto m = mkl::StringUtils::parseQueryString("a=1&b=hello&empty");
    CHECK_EQ(m.at("a"), std::string("1"));
    CHECK_EQ(m.at("b"), std::string("hello"));
    CHECK_EQ(m.at("empty"), std::string(""));
    return true;
}
