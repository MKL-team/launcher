#include "PlatformDetector.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(__APPLE__)
#include <sys/sysctl.h>
#elif defined(__linux__)
#include <sys/utsname.h>
#include <gnu/libc-version.h>
#endif

namespace mkl {

// Windows 下用 RtlGetVersion（不受兼容性 manifest 影响），准确获取真实系统版本
#if defined(_WIN32)
namespace {
typedef LONG(WINAPI* RtlGetVersionFn)(void*);

struct RtlOsVersionInfo {
    ULONG dwOSVersionInfoSize;
    ULONG dwMajorVersion;
    ULONG dwMinorVersion;
    ULONG dwBuildNumber;
    ULONG dwPlatformId;
    WCHAR szCSDVersion[128];
};

bool winVersion(ULONG* major, ULONG* minor) {
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (!ntdll) {
        return false;
    }
    auto fn = reinterpret_cast<RtlGetVersionFn>(GetProcAddress(ntdll, "RtlGetVersion"));
    if (!fn) {
        return false;
    }
    RtlOsVersionInfo info = {};
    info.dwOSVersionInfoSize = sizeof(info);
    if (fn(&info) != 0) {
        return false;
    }
    *major = info.dwMajorVersion;
    *minor = info.dwMinorVersion;
    return true;
}
} // namespace
#endif

static std::string detectArch() {
#if defined(__aarch64__) || defined(_M_ARM64)
    return "arm64";
#elif defined(__x86_64__) || defined(_M_X64)
    return "x64";
#elif defined(__i386__) || defined(_M_IX86)
    return "x86";
#else
    return "unknown";
#endif
}

PlatformInfo PlatformDetector::detect() {
    PlatformInfo info;
    info.arch = detectArch();

#if defined(_WIN32)
    info.os = OSType::Windows;
    ULONG major = 0, minor = 0;
    if (winVersion(&major, &minor)) {
        info.osVersion = std::to_string(major) + "." + std::to_string(minor);
    }
#elif defined(__APPLE__)
    info.os = OSType::macOS;
    char buf[64] = {};
    size_t len = sizeof(buf);
    if (sysctlbyname("kern.osproductversion", buf, &len, nullptr, 0) == 0) {
        info.osVersion = buf;
    }
#elif defined(__ANDROID__)
    info.os = OSType::Android;
    info.androidApiLevel = __ANDROID_API__;
#elif defined(__linux__)
    info.os = OSType::Linux;
    struct utsname uts {};
    if (uname(&uts) == 0) {
        info.osVersion = uts.release;
    }
#ifdef __GLIBC__
    info.glibcVersion = gnu_get_libc_version();
#endif
#else
    info.os = OSType::Unknown;
#endif
    return info;
}

bool PlatformDetector::isWindows7() {
#if defined(_WIN32)
    ULONG major = 0, minor = 0;
    return winVersion(&major, &minor) && major == 6 && minor == 1;
#else
    return false;
#endif
}

bool PlatformDetector::isMacOS14OrLater() {
#if defined(__APPLE__)
    char buf[64] = {};
    size_t len = sizeof(buf);
    if (sysctlbyname("kern.osproductversion", buf, &len, nullptr, 0) == 0) {
        std::string v = buf;
        // "14.x" 及以上
        size_t dot = v.find('.');
        std::string majorStr = (dot == std::string::npos) ? v : v.substr(0, dot);
        try {
            return std::stoi(majorStr) >= 14;
        } catch (...) {
            return false;
        }
    }
    return false;
#else
    return false;
#endif
}

bool PlatformDetector::isMobile() {
    PlatformInfo info = detect();
    return info.os == OSType::Android || info.os == OSType::iOS;
}

} // namespace mkl
