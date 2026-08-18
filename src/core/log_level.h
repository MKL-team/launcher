#ifndef MKL_LOG_LEVEL_H
#define MKL_LOG_LEVEL_H

#ifdef __cplusplus
extern "C" {
#endif

/* 日志级别（规格 R2：基础层公共类型） */
typedef enum mkl_log_level {
    MKL_LOG_LEVEL_TRACE = 0,
    MKL_LOG_LEVEL_DEBUG,
    MKL_LOG_LEVEL_INFO,
    MKL_LOG_LEVEL_WARN,
    MKL_LOG_LEVEL_ERROR,
    MKL_LOG_LEVEL_FATAL
} mkl_log_level;

const char* mkl_log_level_name(mkl_log_level level);

#ifdef __cplusplus
}
#endif

#endif /* MKL_LOG_LEVEL_H */
