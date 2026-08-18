#include "json.h"

#include "test_util.h"

static const char* kJson =
    "{\"version\":{\"id\":\"1.21.1\",\"type\":\"release\"},\"enabled\":true,\"count\":42,"
    "\"tags\":[\"a\",\"b\",\"c\"],\"map\":{\"k1\":\"v1\",\"k2\":\"v2\"},"
    "\"nested\":{\"arr\":[{\"name\":\"client\",\"size\":3},{\"name\":\"server\"}]}}";

MKL_TEST(json_valid) {
    CHECK(mkl_json_is_valid(kJson));
    CHECK(!mkl_json_is_valid("{not json"));
    CHECK(!mkl_json_is_valid(""));
    return 1;
}

MKL_TEST(json_get) {
    char out[128];
    mkl_json_get_string(kJson, "version.id", out, sizeof(out), "def");
    CHECK_EQ_STR(out, "1.21.1");
    mkl_json_get_string(kJson, "version.type", out, sizeof(out), "def");
    CHECK_EQ_STR(out, "release");
    mkl_json_get_string(kJson, "missing.key", out, sizeof(out), "def");
    CHECK_EQ_STR(out, "def");

    CHECK_EQ_INT(mkl_json_get_int(kJson, "count", -1), 42);
    CHECK_EQ_INT(mkl_json_get_int(kJson, "missing", -1), -1);
    CHECK(mkl_json_get_bool(kJson, "enabled", 0));
    CHECK(!mkl_json_get_bool(kJson, "missing", 0));
    return 1;
}

MKL_TEST(json_array_map) {
    char items[MKL_JSON_MAX_ITEMS][MKL_JSON_ITEM_LEN];
    int n = mkl_json_get_string_array(kJson, "tags", items, MKL_JSON_MAX_ITEMS);
    CHECK_EQ_INT(n, 3);
    CHECK_EQ_STR(items[0], "a");
    CHECK_EQ_STR(items[2], "c");

    char keys[MKL_JSON_MAX_ITEMS][MKL_JSON_KEY_LEN];
    char vals[MKL_JSON_MAX_ITEMS][MKL_JSON_ITEM_LEN];
    int m = mkl_json_get_string_map(kJson, "map", keys, vals, MKL_JSON_MAX_ITEMS);
    CHECK_EQ_INT(m, 2);
    CHECK_EQ_STR(keys[0], "k1");
    CHECK_EQ_STR(vals[0], "v1");

    char out[128];
    mkl_json_get_string(kJson, "nested.arr.0.name", out, sizeof(out), "");
    CHECK_EQ_STR(out, "client");
    CHECK_EQ_INT(mkl_json_get_int(kJson, "nested.arr.0.size", -1), 3);
    return 1;
}

MKL_TEST(json_escapes) {
    /* \u0041 == 'A'（纯 ASCII，避免 Windows 代码页差异） */
    const char* j = "{\"s\":\"line\\nbreak \\\"quote\\\" \\u0041\"}";
    CHECK(mkl_json_is_valid(j));
    char out[128];
    mkl_json_get_string(j, "s", out, sizeof(out), "");
    CHECK_EQ_STR(out, "line\nbreak \"quote\" A");
    return 1;
}

MKL_TEST(json_serialize) {
    char out[512];
    const char* keys[2] = {"name", "ver"};
    const char* vals[2] = {"mkl", "0.0.2"};
    CHECK(mkl_json_serialize_object(keys, vals, 2, out, sizeof(out)) == 0);
    CHECK(mkl_json_is_valid(out));
    char v[128];
    mkl_json_get_string(out, "name", v, sizeof(v), "");
    CHECK_EQ_STR(v, "mkl");

    const char* items[2] = {"x", "y"};
    CHECK(mkl_json_serialize_string_array(items, 2, out, sizeof(out)) == 0);
    CHECK(mkl_json_is_valid(out));
    char arr[MKL_JSON_MAX_ITEMS][MKL_JSON_ITEM_LEN];
    int n = mkl_json_get_string_array(out, "", arr, MKL_JSON_MAX_ITEMS);
    CHECK_EQ_INT(n, 2);
    CHECK_EQ_STR(arr[1], "y");
    return 1;
}
