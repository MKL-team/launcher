#if !defined(_WIN32) && !defined(__APPLE__)
#define _POSIX_C_SOURCE 200809L
#endif

#include "platform_detector.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(__APPLE__)
#include <sys/sysctl.h>
#elif defined(__linux__)
#include <gnu/libc-version.h>
#include <sys/utsname.h>
#endif

#if defined(_WIN32)
typedef LONG(WINAPI* RtlGetVersionFn)(void*);

typedef struct RtlOsVersionInfo {
    ULONG dwOSVersionInfoSize;
    ULONG dwMajorVersion;
    ULONG dwMinorVersion;
    ULONG dwBuildNumber;
    ULONG dwPlatformId;
    WCHAR szCSDVersion[128];
} RtlOsVersionInfo;

static int win_version(ULONG* major, ULONG* minor) {
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (!ntdll) {
        return 0;
    }
    RtlGetVersionFn fn = (RtlGetVersionFn)(void*)GetProcAddress(ntdll, "RtlGetVersion");
    if (!fn) {
        return 0;
    }
    RtlOsVersionInfo info;
    memset(&info, 0, sizeof(info));
    info.dwOSVersionInfoSize = sizeof(info);
    if (fn(&info) != 0) {
        return 0;
    }
    *major = info.dwMajorVersion;
    *minor = info.dwMinorVersion;
    return 1;
}
#endif

static const char* detect_arch(void) {
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

int mkl_platform_detect(mkl_platform_info* out) {
    if (!out) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    snprintf(out->arch, sizeof(out->arch), "%s", detect_arch());

#if defined(_WIN32)
    out->os = MKL_OS_WINDOWS;
    ULONG major = 0, minor = 0;
    if (win_version(&major, &minor)) {
        snprintf(out->os_version, sizeof(out->os_version), "%lu.%lu",
                 (unsigned long)major, (unsigned long)minor);
    }
#elif defined(__APPLE__)
    out->os = MKL_OS_MACOS;
    {
        char buf[64] = {0};
        size_t len = sizeof(buf);
        if (sysctlbyname("kern.osproductversion", buf, &len, NULL, 0) == 0) {
            snprintf(out->os_version, sizeof(out->os_version), "%s", buf);
        }
    }
#elif defined(__ANDROID__)
    out->os = MKL_OS_ANDROID;
    out->android_api_level = __ANDROID_API__;
#elif defined(__linux__)
    out->os = MKL_OS_LINUX;
    {
        struct utsname uts;
        memset(&uts, 0, sizeof(uts));
        if (uname(&uts) == 0) {
            snprintf(out->os_version, sizeof(out->os_version), "%s", uts.release);
        }
    }
#ifdef __GLIBC__
    {
        const char* g = gnu_get_libc_version();
        if (g) {
            snprintf(out->glibc_version, sizeof(out->glibc_version), "%s", g);
        }
    }
#endif
#else
    out->os = MKL_OS_UNKNOWN;
#endif
    return 0;
}

int mkl_platform_is_windows7(void) {
#if defined(_WIN32)
    ULONG major = 0, minor = 0;
    return win_version(&major, &minor) && major == 6 && minor == 1;
#else
    return 0;
#endif
}

int mkl_platform_is_macos14_or_later(void) {
#if defined(__APPLE__)
    char buf[64] = {0};
    size_t len = sizeof(buf);
    if (sysctlbyname("kern.osproductversion", buf, &len, NULL, 0) == 0) {
        char* dot = strchr(buf, '.');
        char major_str[8] = {0};
        if (dot) {
            size_t n = (size_t)(dot - buf);
            if (n >= sizeof(major_str)) {
                n = sizeof(major_str) - 1;
            }
            memcpy(major_str, buf, n);
        } else {
            snprintf(major_str, sizeof(major_str), "%s", buf);
        }
        int major = atoi(major_str);
        return major >= 14;
    }
    return 0;
#else
    return 0;
#endif
}

int mkl_platform_is_mobile(void) {
    mkl_platform_info info;
    if (mkl_platform_detect(&info) != 0) {
        return 0;
    }
    return info.os == MKL_OS_ANDROID || info.os == MKL_OS_IOS;
}
