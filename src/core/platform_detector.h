#ifndef MKL_PLATFORM_DETECTOR_H
#define MKL_PLATFORM_DETECTOR_H

#ifdef __cplusplus
extern "C" {
#endif

/* 操作系统类型（规格 R2：模块 2 PlatformDetector） */
typedef enum mkl_os_type {
    MKL_OS_WINDOWS = 0,
    MKL_OS_MACOS,
    MKL_OS_LINUX,
    MKL_OS_ANDROID,
    MKL_OS_IOS,
    MKL_OS_UNKNOWN
} mkl_os_type;

typedef struct mkl_platform_info {
    mkl_os_type os;
    char os_version[64];
    char arch[16];          /* "x64", "arm64" */
    char glibc_version[16]; /* Linux only */
    int android_api_level;  /* Android only */
} mkl_platform_info;

/* 返回 0 成功 */
int mkl_platform_detect(mkl_platform_info* out);
int mkl_platform_is_windows7(void);
int mkl_platform_is_macos14_or_later(void);
int mkl_platform_is_mobile(void);

#ifdef __cplusplus
}
#endif

#endif /* MKL_PLATFORM_DETECTOR_H */
