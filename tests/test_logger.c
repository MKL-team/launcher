#include "file_utils.h"
#include "logger.h"

#include "test_util.h"

MKL_TEST(logger_file) {
    char tdir[512];
    char dir[1024];
    char path[1100];
    mkl_file_temp_directory(tdir, sizeof(tdir));
    const char* parts[2] = {tdir, "mkl_test_logger"};
    mkl_file_join_path(parts, 2, dir, sizeof(dir));
    mkl_file_delete_directory(dir);
    mkl_file_create_directories(dir);
    const char* file_parts[2] = {dir, "test.log"};
    mkl_file_join_path(file_parts, 2, path, sizeof(path));

    mkl_logger_init(path);
    mkl_logger_set_console(0);
    mkl_logger_log(MKL_LOG_LEVEL_INFO, "test_logger.c", 10, "hello logger");
    mkl_logger_flush();

    CHECK(mkl_file_exists(path));
    char content[2048];
    long n = mkl_file_read_all_text(path, content, sizeof(content));
    CHECK(n > 0);
    CHECK(strstr(content, "hello logger") != NULL);
    CHECK(strstr(content, "INFO") != NULL);

    mkl_file_delete_directory(dir);
    return 1;
}

static void log_count_cb(mkl_log_level level, const char* line, void* userdata) {
    (void)level;
    (void)line;
    int* p = (int*)userdata;
    (*p)++;
}

MKL_TEST(logger_subscribe) {
    int received = 0;
    mkl_logger_subscribe(log_count_cb, &received);
    mkl_logger_log(MKL_LOG_LEVEL_DEBUG, "t", 1, "subscribe check");
    CHECK(received >= 1);
    return 1;
}

MKL_TEST(logger_ring) {
    for (int i = 0; i < 60; ++i) {
        mkl_logger_log(MKL_LOG_LEVEL_DEBUG, "ring", i, "line %d", i);
    }
    char lines[50][512];
    int count = mkl_logger_recent_lines(lines, 50);
    CHECK_EQ_INT(count, 50);
    CHECK(strstr(lines[0], "line 10") != NULL);
    CHECK(strstr(lines[49], "line 59") != NULL);
    return 1;
}
