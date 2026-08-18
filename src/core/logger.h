#ifndef MKL_LOGGER_H
#define MKL_LOGGER_H

#include "log_level.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 实时日志订阅回调 */
typedef void (*mkl_log_sink_fn)(mkl_log_level level, const char* line, void* userdata);

/* FATAL 弹窗处理器（由平台垫片注入；默认仅控制台/文件输出） */
typedef void (*mkl_fatal_handler_fn)(const char* const* last_lines, int count, void* userdata);

/* 日志文件路径（logs/MKL_日期.log）；NULL 表示不写文件 */
int mkl_logger_init(const char* file_path); /* 返回 0 成功 */
void mkl_logger_shutdown(void);

void mkl_logger_set_console(int enable);
void mkl_logger_set_min_console_level(mkl_log_level level);
void mkl_logger_log(mkl_log_level level, const char* file, int line, const char* fmt, ...);
void mkl_logger_flush(void);

void mkl_logger_subscribe(mkl_log_sink_fn sink, void* userdata);
/* 最近 N 行（FATAL 弹窗需要最近 50 行）；返回实际拷贝行数 */
int mkl_logger_recent_lines(char (*out)[512], int max_lines);
void mkl_logger_set_fatal_handler(mkl_fatal_handler_fn handler, void* userdata);

#define MKL_LOG(level, ...) mkl_logger_log(level, __FILE__, __LINE__, __VA_ARGS__)
#define MKL_INFO(...)  MKL_LOG(MKL_LOG_LEVEL_INFO,  __VA_ARGS__)
#define MKL_WARN(...)  MKL_LOG(MKL_LOG_LEVEL_WARN,  __VA_ARGS__)
#define MKL_ERROR(...) MKL_LOG(MKL_LOG_LEVEL_ERROR, __VA_ARGS__)
#define MKL_FATAL(...) MKL_LOG(MKL_LOG_LEVEL_FATAL, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* MKL_LOGGER_H */
