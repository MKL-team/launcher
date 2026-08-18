#include "JsonUtils.h"

#include <map>
#include <string>
#include <vector>

#include "test_util.h"

namespace {

const std::string kJson =
    R"({"version":{"id":"1.21.1","type":"release","channel":"stable"},"enabled":true,"count":42,"tags":["a","b","c"],"map":{"k1":"v1","k2":"v2"},"nested":{"arr":[{"name":"client","size":3},{"name":"server"}]}})";

} // namespace

MKL_TEST(json_is_valid) {
    CHECK(mkl::JsonUtils::isJson(kJson));
    CHECK(!mkl::JsonUtils::isJson("{not json"));
    CHECK(!mkl::JsonUtils::isJson(""));
    return true;
}

MKL_TEST(json_get_string) {
    CHECK_EQ(mkl::JsonUtils::getString(kJson, "version.id"), std::string("1.21.1"));
    CHECK_EQ(mkl::JsonUtils::getString(kJson, "version.type"), std::string("release"));
    CHECK_EQ(mkl::JsonUtils::getString(kJson, "missing.key", "def"), std::string("def"));
    return true;
}

MKL_TEST(json_get_int_and_bool) {
    CHECK_EQ(mkl::JsonUtils::getInt(kJson, "count"), 42);
    CHECK_EQ(mkl::JsonUtils::getInt(kJson, "missing", -1), -1);
    CHECK(mkl::JsonUtils::getBool(kJson, "enabled"));
    CHECK(!mkl::JsonUtils::getBool(kJson, "missing", false));
    return true;
}

MKL_TEST(json_get_string_array) {
    const auto arr = mkl::JsonUtils::getStringArray(kJson, "tags");
    CHECK_EQ(arr.size(), 3U);
    CHECK_EQ(arr[0], std::string("a"));
    CHECK_EQ(arr[2], std::string("c"));
    return true;
}

MKL_TEST(json_get_string_map) {
    const auto m = mkl::JsonUtils::getStringMap(kJson, "map");
    CHECK_EQ(m.size(), 2U);
    CHECK_EQ(m.at("k1"), std::string("v1"));
    CHECK_EQ(m.at("k2"), std::string("v2"));
    return true;
}

MKL_TEST(json_array_index_path) {
    CHECK_EQ(mkl::JsonUtils::getString(kJson, "nested.arr.0.name"), std::string("client"));
    CHECK_EQ(mkl::JsonUtils::getInt(kJson, "nested.arr.0.size"), 3);
    return true;
}

MKL_TEST(json_escapes) {
    const std::string j = R"({"s":"line\nbreak \"quote\" \u4e2d"}")";
    CHECK(mkl::JsonUtils::isJson(j));
    CHECK_EQ(mkl::JsonUtils::getString(j, "s"), std::string("line\nbreak \"quote\" \u4e2d"));
    return true;
}

MKL_TEST(json_serialize_roundtrip) {
    std::map<std::string, std::string> obj{{"name", "mkl"}, {"ver", "0.0.1"}};
    const std::string ser = mkl::JsonUtils::serializeObject(obj);
    CHECK(mkl::JsonUtils::isJson(ser));
    CHECK_EQ(mkl::JsonUtils::getString(ser, "name"), std::string("mkl"));
    CHECK_EQ(mkl::JsonUtils::getString(ser, "ver"), std::string("0.0.1"));

    const std::string arrSer = mkl::JsonUtils::serializeStringArray({"x", "y"});
    CHECK(mkl::JsonUtils::isJson(arrSer));
    const auto arr = mkl::JsonUtils::getStringArray(arrSer, "");
    CHECK_EQ(arr.size(), 2U);
    CHECK_EQ(arr[1], std::string("y"));
    return true;
}
