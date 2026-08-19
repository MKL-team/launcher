#include "test_util.h"

int mkl_test_failures = 0;

/* 各测试函数声明（新增测试需在此登记） */
int mkl_test_logger_file(void);
int mkl_test_logger_subscribe(void);
int mkl_test_logger_ring(void);
int mkl_test_platform_basic(void);
int mkl_test_platform_arch(void);
int mkl_test_file_write_read(void);
int mkl_test_file_copy_move_delete(void);
int mkl_test_file_sha1(void);
int mkl_test_file_format_size(void);
int mkl_test_file_join_path(void);
int mkl_test_string_trim_split(void);
int mkl_test_string_replace_case(void);
int mkl_test_string_query(void);
int mkl_test_json_valid(void);
int mkl_test_json_get(void);
int mkl_test_json_array_map(void);
int mkl_test_json_escapes(void);
int mkl_test_json_serialize(void);

typedef struct {
    const char* name;
    int (*fn)(void);
} mkl_test_case;

static mkl_test_case tests[] = {
    {"logger_file", mkl_test_logger_file},
    {"logger_subscribe", mkl_test_logger_subscribe},
    {"logger_ring", mkl_test_logger_ring},
    {"platform_basic", mkl_test_platform_basic},
    {"platform_arch", mkl_test_platform_arch},
    {"file_write_read", mkl_test_file_write_read},
    {"file_copy_move_delete", mkl_test_file_copy_move_delete},
    {"file_sha1", mkl_test_file_sha1},
    {"file_format_size", mkl_test_file_format_size},
    {"file_join_path", mkl_test_file_join_path},
    {"string_trim_split", mkl_test_string_trim_split},
    {"string_replace_case", mkl_test_string_replace_case},
    {"string_query", mkl_test_string_query},
    {"json_valid", mkl_test_json_valid},
    {"json_get", mkl_test_json_get},
    {"json_array_map", mkl_test_json_array_map},
    {"json_escapes", mkl_test_json_escapes},
    {"json_serialize", mkl_test_json_serialize},
};

int main(void) {
    /* stdout 设为无缓冲：崩溃时也能看到已通过的测试进度 */
    setvbuf(stdout, NULL, _IONBF, 0);
    int failed = 0;
    size_t n = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < n; ++i) {
        fprintf(stderr, "[RUN ] %s\n", tests[i].name);
        int before = mkl_test_failures;
        int ok = tests[i].fn();
        int after = mkl_test_failures;
        if (!ok || after != before) {
            fprintf(stderr, "[FAIL] %s\n", tests[i].name);
            failed++;
        } else {
            fprintf(stderr, "[ OK ] %s\n", tests[i].name);
        }
    }
    if (failed == 0 && mkl_test_failures == 0) {
        fprintf(stderr, "ALL TESTS PASSED\n");
        return 0;
    }
    fprintf(stderr, "TESTS FAILED (assertions=%d)\n", mkl_test_failures);
    return 1;
}
