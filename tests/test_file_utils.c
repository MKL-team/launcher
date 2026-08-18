#include "file_utils.h"

#include "test_util.h"

static void make_test_dir(char* dir, size_t dir_sz, const char* sub) {
    char tdir[512];
    mkl_file_temp_directory(tdir, sizeof(tdir));
    const char* parts[2] = {tdir, sub};
    mkl_file_join_path(parts, 2, dir, dir_sz);
    mkl_file_delete_directory(dir);
    mkl_file_create_directories(dir);
}

MKL_TEST(file_write_read) {
    char dir[1024];
    char path[1100];
    make_test_dir(dir, sizeof(dir), "mkl_test_fs");
    const char* parts[2] = {dir, "hello.txt"};
    mkl_file_join_path(parts, 2, path, sizeof(path));

    CHECK(mkl_file_write_all_text(path, "hello mkl") == 0);
    CHECK(mkl_file_exists(path));
    char content[128];
    long n = mkl_file_read_all_text(path, content, sizeof(content));
    CHECK_EQ_INT(n, 9);
    CHECK_EQ_STR(content, "hello mkl");
    CHECK_EQ_INT(mkl_file_size(path), 9);

    mkl_file_delete_directory(dir);
    return 1;
}

MKL_TEST(file_copy_move_delete) {
    char dir[1024];
    char src[1100], dst[1100], moved[1100];
    make_test_dir(dir, sizeof(dir), "mkl_test_fs2");
    const char* p1[2] = {dir, "a.txt"};
    const char* p2[2] = {dir, "b.txt"};
    const char* p3[2] = {dir, "c.txt"};
    mkl_file_join_path(p1, 2, src, sizeof(src));
    mkl_file_join_path(p2, 2, dst, sizeof(dst));
    mkl_file_join_path(p3, 2, moved, sizeof(moved));
    mkl_file_write_all_text(src, "data");

    CHECK(mkl_file_copy(src, dst) == 0);
    CHECK(mkl_file_exists(dst));
    char content[64];
    mkl_file_read_all_text(dst, content, sizeof(content));
    CHECK_EQ_STR(content, "data");

    CHECK(mkl_file_move(dst, moved) == 0);
    CHECK(!mkl_file_exists(dst));
    CHECK(mkl_file_exists(moved));

    CHECK(mkl_file_delete(src) == 0);
    CHECK(mkl_file_delete(moved) == 0);
    CHECK(!mkl_file_exists(src));

    mkl_file_delete_directory(dir);
    return 1;
}

MKL_TEST(file_sha1) {
    char dir[1024];
    char path[1100];
    make_test_dir(dir, sizeof(dir), "mkl_test_fs3");
    const char* parts[2] = {dir, "abc.txt"};
    mkl_file_join_path(parts, 2, path, sizeof(path));
    mkl_file_write_all_text(path, "abc");

    char sha[41];
    CHECK(mkl_file_sha1(path, sha) == 0);
    CHECK_EQ_STR(sha, "a9993e364706816aba3e25717850c26c9cd0d89d");

    mkl_file_delete_directory(dir);
    return 1;
}

MKL_TEST(file_format_size) {
    char buf[64];
    mkl_file_format_size(0, buf, sizeof(buf));
    CHECK_EQ_STR(buf, "0 B");
    mkl_file_format_size(1024, buf, sizeof(buf));
    CHECK_EQ_STR(buf, "1.0 KB");
    mkl_file_format_size(1536, buf, sizeof(buf));
    CHECK_EQ_STR(buf, "1.5 KB");
    mkl_file_format_size(1048576, buf, sizeof(buf));
    CHECK_EQ_STR(buf, "1.0 MB");
    return 1;
}

MKL_TEST(file_join_path) {
    const char* parts[3] = {"a", "b", "c"};
    char out[256];
    mkl_file_join_path(parts, 3, out, sizeof(out));
    CHECK(out[0] != '\0');
    CHECK(strstr(out, "c") != NULL);
    return 1;
}
