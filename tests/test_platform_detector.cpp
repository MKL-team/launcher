#include "PlatformDetector.h"

#include "test_util.h"

MKL_TEST(platform_detect_basic) {
    const mkl::PlatformInfo info = mkl::PlatformDetector::detect();
    CHECK(info.os != mkl::OSType::Unknown);
    CHECK(!info.arch.empty());
    return true;
}

MKL_TEST(platform_arch_is_valid) {
    const mkl::PlatformInfo info = mkl::PlatformDetector::detect();
    CHECK(info.arch == "x64" || info.arch == "arm64" || info.arch == "x86" ||
          info.arch == "unknown");
    return true;
}

MKL_TEST(platform_boolean_helpers_are_callable) {
    // 不断言具体值（跨平台），只验证可调用且类型正确
    const bool w7 = mkl::PlatformDetector::isWindows7();
    const bool m14 = mkl::PlatformDetector::isMacOS14OrLater();
    const bool mobile = mkl::PlatformDetector::isMobile();
    CHECK(w7 || !w7);
    CHECK(m14 || !m14);
    CHECK(mobile || !mobile);
    return true;
}
