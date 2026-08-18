#include "platform_detector.h"

#include "test_util.h"

MKL_TEST(platform_basic) {
    mkl_platform_info info;
    CHECK(mkl_platform_detect(&info) == 0);
    CHECK(info.os != MKL_OS_UNKNOWN);
    CHECK(info.arch[0] != '\0');
    return 1;
}

MKL_TEST(platform_arch) {
    mkl_platform_info info;
    mkl_platform_detect(&info);
    CHECK(strcmp(info.arch, "x64") == 0 || strcmp(info.arch, "arm64") == 0 ||
          strcmp(info.arch, "x86") == 0 || strcmp(info.arch, "unknown") == 0);
    return 1;
}
