#ifndef MKL_ERROR_H
#define MKL_ERROR_H

#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 错误码（规格 R2 5.1：C 错误处理） */
typedef enum mkl_error_code {
    MKL_OK = 0,
    MKL_ERR_INVALID_OPTIONS,
    MKL_ERR_NOT_FOUND,
    MKL_ERR_IO,
    MKL_ERR_NETWORK,
    MKL_ERR_DOWNLOAD_FAILED,
    MKL_ERR_VERIFICATION_FAILED,
    MKL_ERR_JAVA_NOT_FOUND,
    MKL_ERR_PROCESS_FAILED,
    MKL_ERR_AUTH_FAILED,
    MKL_ERR_STORAGE,
    MKL_ERR_PLUGIN,
    MKL_ERR_UPDATE_FAILED,
    MKL_ERR_UNSUPPORTED,
    MKL_ERR_UNKNOWN
} mkl_error_code;

/* 错误结构：包含上下文（文件、函数、行号、系统 errno） */
typedef struct mkl_error {
    mkl_error_code code;
    char message[512];
    char file[128];
    char function[128];
    int line;
    int system_errno;
} mkl_error;

/* 便捷初始化（自动填充文件/函数/行号/errno） */
#define MKL_ERROR_INIT(e, c, m)                                              \
    do {                                                                     \
        (e)->code = (c);                                                     \
        (void)snprintf((e)->message, sizeof((e)->message), "%s", (m));       \
        (void)snprintf((e)->file, sizeof((e)->file), "%s", __FILE__);        \
        (void)snprintf((e)->function, sizeof((e)->function), "%s", __func__);\
        (e)->line = __LINE__;                                                \
        (e)->system_errno = errno;                                           \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif /* MKL_ERROR_H */
