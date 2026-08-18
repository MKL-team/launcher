#include "FileUtils.h"

#include <string>

#include "test_util.h"

MKL_TEST(file_write_read_exists) {
    const std::string dir = mkl::FileUtils::getTempDirectory() + "/mkl_test_fs";
    mkl::FileUtils::deleteDirectory(dir);
    mkl::FileUtils::createDirectories(dir);

    const std::string path = dir + "/hello.txt";
    CHECK(mkl::FileUtils::writeAllText(path, "hello mkl"));
    CHECK(mkl::FileUtils::exists(path));
    CHECK_EQ(mkl::FileUtils::readAllText(path), std::string("hello mkl"));
    CHECK_EQ(mkl::FileUtils::getFileSize(path), 9ULL);

    mkl::FileUtils::deleteDirectory(dir);
    return true;
}

MKL_TEST(file_copy_move_delete) {
    const std::string dir = mkl::FileUtils::getTempDirectory() + "/mkl_test_fs2";
    mkl::FileUtils::deleteDirectory(dir);
    mkl::FileUtils::createDirectories(dir);

    const std::string src = dir + "/a.txt";
    const std::string dst = dir + "/b.txt";
    const std::string moved = dir + "/c.txt";
    mkl::FileUtils::writeAllText(src, "data");

    CHECK(mkl::FileUtils::copyFile(src, dst));
    CHECK(mkl::FileUtils::exists(dst));
    CHECK_EQ(mkl::FileUtils::readAllText(dst), std::string("data"));

    CHECK(mkl::FileUtils::moveFile(dst, moved));
    CHECK(!mkl::FileUtils::exists(dst));
    CHECK(mkl::FileUtils::exists(moved));

    CHECK(mkl::FileUtils::deleteFile(src));
    CHECK(mkl::FileUtils::deleteFile(moved));
    CHECK(!mkl::FileUtils::exists(src));

    mkl::FileUtils::deleteDirectory(dir);
    return true;
}

MKL_TEST(file_sha1_known_vector) {
    const std::string dir = mkl::FileUtils::getTempDirectory() + "/mkl_test_fs3";
    mkl::FileUtils::deleteDirectory(dir);
    mkl::FileUtils::createDirectories(dir);

    const std::string path = dir + "/abc.txt";
    mkl::FileUtils::writeAllText(path, "abc");
    // SHA-1("abc") 标准向量
    CHECK_EQ(mkl::FileUtils::getFileSha1(path),
             std::string("a9993e364706816aba3e25717850c26c9cd0d89d"));

    mkl::FileUtils::deleteDirectory(dir);
    return true;
}

MKL_TEST(file_format_size) {
    CHECK_EQ(mkl::FileUtils::formatFileSize(0), std::string("0 B"));
    CHECK_EQ(mkl::FileUtils::formatFileSize(1024), std::string("1.0 KB"));
    CHECK_EQ(mkl::FileUtils::formatFileSize(1536), std::string("1.5 KB"));
    CHECK_EQ(mkl::FileUtils::formatFileSize(1048576), std::string("1.0 MB"));
    return true;
}

MKL_TEST(file_join_path) {
    const std::string p = mkl::FileUtils::joinPath({"a", "b", "c"});
    CHECK(!p.empty());
    CHECK(p.find("c") != std::string::npos);
    return true;
}
