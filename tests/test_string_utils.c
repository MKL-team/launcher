#include "string_utils.h"

#include "test_util.h"

MKL_TEST(string_trim_split) {
    char out[128];
    mkl_string_trim("  hello  ", out, sizeof(out));
    CHECK_EQ_STR(out, "hello");
    mkl_string_trim("\t\n hello \n", out, sizeof(out));
    CHECK_EQ_STR(out, "hello");
    mkl_string_trim("", out, sizeof(out));
    CHECK_EQ_STR(out, "");

    char parts[MKL_STRING_MAX_PARTS][MKL_STRING_PART_LEN];
    int n = mkl_string_split("a,b,c", ',', parts, MKL_STRING_MAX_PARTS);
    CHECK_EQ_INT(n, 3);
    CHECK_EQ_STR(parts[0], "a");
    CHECK_EQ_STR(parts[2], "c");
    return 1;
}

MKL_TEST(string_replace_case) {
    char out[256];
    mkl_string_replace("hello world world", "world", "mkl", out, sizeof(out));
    CHECK_EQ_STR(out, "hello mkl mkl");
    mkl_string_replace("abc", "x", "y", out, sizeof(out));
    CHECK_EQ_STR(out, "abc");

    CHECK(mkl_string_startswith("launcher.exe", "launcher"));
    CHECK(!mkl_string_startswith("launcher.exe", "x"));
    CHECK(mkl_string_endswith("launcher.exe", ".exe"));
    CHECK(!mkl_string_endswith("launcher.exe", ".zip"));

    mkl_string_tolower("AbC", out, sizeof(out));
    CHECK_EQ_STR(out, "abc");
    mkl_string_toupper("aBc", out, sizeof(out));
    CHECK_EQ_STR(out, "ABC");

    mkl_string_format(out, sizeof(out), "%s-%d", "v", 7);
    CHECK_EQ_STR(out, "v-7");
    return 1;
}

MKL_TEST(string_query) {
    char keys[MKL_STRING_MAX_PARTS][MKL_STRING_KEY_LEN];
    char vals[MKL_STRING_MAX_PARTS][MKL_STRING_PART_LEN];
    int n = mkl_string_parse_query("a=1&b=hello&empty", keys, vals, MKL_STRING_MAX_PARTS);
    CHECK_EQ_INT(n, 3);
    CHECK_EQ_STR(keys[0], "a");
    CHECK_EQ_STR(vals[0], "1");
    CHECK_EQ_STR(keys[1], "b");
    CHECK_EQ_STR(vals[1], "hello");
    CHECK_EQ_STR(keys[2], "empty");
    CHECK_EQ_STR(vals[2], "");
    return 1;
}
