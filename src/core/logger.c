/* 需要 POSIX 声明（localtime_r）：-std=c11 下默认不可见 */
#if !defined(_WIN32)
#define _POSIX_C_SOURCE 200809L
#endif

#include "logger.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#define MKL_RING_CAPACITY 50
#define MKL_LINE_LEN 512

/* ---- 平台互斥 ---- */
#ifdef _WIN32
typedef CRITICAL_SECTION mkl_mutex;
#define mkl_mutex_init(m) InitializeCriticalSection(m)
#define mkl_mutex_lock(m) EnterCriticalSection(m)
#define mkl_mutex_unlock(m) LeaveCriticalSection(m)
#define mkl_mutex_destroy(m) DeleteCriticalSection(m)
#else
typedef pthread_mutex_t mkl_mutex;
#define mkl_mutex_init(m) pthread_mutex_init(m, NULL)
#define mkl_mutex_lock(m) pthread_mutex_lock(m)
#define mkl_mutex_unlock(m) pthread_mutex_unlock(m)
#define mkl_mutex_destroy(m) pthread_mutex_destroy(m)
#endif

typedef struct mkl_log_sink {
    mkl_log_sink_fn fn;
    void* userdata;
} mkl_log_sink;

typedef struct mkl_logger_state {
    mkl_mutex mutex;
    int console;
    mkl_log_level min_console;
    FILE* file;
    char ring[MKL_RING_CAPACITY][MKL_LINE_LEN];
    int ring_count;
    mkl_log_sink sinks[16];
    int sink_count;
    mkl_fatal_handler_fn fatal_handler;
    void* fatal_userdata;
    int initialized;
} mkl_logger_state;

static mkl_logger_state s_logger;

const char* mkl_log_level_name(mkl_log_level level) {
    switch (level) {
        case MKL_LOG_LEVEL_TRACE: return "TRACE";
        case MKL_LOG_LEVEL_DEBUG: return "DEBUG";
        case MKL_LOG_LEVEL_INFO:  return "INFO";
        case MKL_LOG_LEVEL_WARN:  return "WARN";
        case MKL_LOG_LEVEL_ERROR: return "ERROR";
        case MKL_LOG_LEVEL_FATAL: return "FATAL";
    }
    return "UNKNOWN";
}

static void mkl_now_string(char* out, size_t out_sz) {
    time_t t = time(NULL);
    struct tm tm;
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    strftime(out, out_sz, "%Y-%m-%d %H:%M:%S", &tm);
}

static void mkl_format_line(mkl_log_level level, const char* file, int line,
                            const char* msg, char* out, size_t out_sz) {
    char now[32];
    mkl_now_string(now, sizeof(now));
    snprintf(out, out_sz, "[%s] [%s] %s:%d %s", now, mkl_log_level_name(level), file, line,
             msg);
}

int mkl_logger_init(const char* file_path) {
    mkl_mutex_init(&s_logger.mutex);
    s_logger.console = 0;
    s_logger.min_console = MKL_LOG_LEVEL_INFO;
    s_logger.file = NULL;
    s_logger.ring_count = 0;
    s_logger.sink_count = 0;
    s_logger.fatal_handler = NULL;
    s_logger.fatal_userdata = NULL;
    s_logger.initialized = 1;
    if (file_path && *file_path) {
        s_logger.file = fopen(file_path, "a");
        if (!s_logger.file) {
            /* 目录不存在等：静默降级为仅控制台 */
        }
    }
    return 0;
}

void mkl_logger_shutdown(void) {
    if (!s_logger.initialized) {
        return;
    }
    mkl_logger_flush();
    if (s_logger.file) {
        fclose(s_logger.file);
        s_logger.file = NULL;
    }
    mkl_mutex_destroy(&s_logger.mutex);
    s_logger.initialized = 0;
}

void mkl_logger_set_console(int enable) {
    mkl_mutex_lock(&s_logger.mutex);
    s_logger.console = enable;
    mkl_mutex_unlock(&s_logger.mutex);
}

void mkl_logger_set_min_console_level(mkl_log_level level) {
    mkl_mutex_lock(&s_logger.mutex);
    s_logger.min_console = level;
    mkl_mutex_unlock(&s_logger.mutex);
}

void mkl_logger_log(mkl_log_level level, const char* file, int line, const char* fmt, ...) {
    if (!s_logger.initialized) {
        mkl_logger_init(NULL);
    }
    char msg[MKL_LINE_LEN];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);

    char text[MKL_LINE_LEN];
    mkl_format_line(level, file, line, msg, text, sizeof(text));

    int fatal = (level == MKL_LOG_LEVEL_FATAL);
    mkl_log_sink sinks[16];
    int sink_count = 0;
    char ring_copy[MKL_RING_CAPACITY][MKL_LINE_LEN];
    int ring_copy_count = 0;
    mkl_fatal_handler_fn fatal_handler = NULL;
    void* fatal_userdata = NULL;

    mkl_mutex_lock(&s_logger.mutex);
    if (s_logger.file) {
        fprintf(s_logger.file, "%s\n", text);
    }
    if (s_logger.console && level >= s_logger.min_console) {
        printf("%s\n", text);
    }
    /* 环形缓冲：保留最近 50 行 */
    if (s_logger.ring_count < MKL_RING_CAPACITY) {
        snprintf(s_logger.ring[s_logger.ring_count], MKL_LINE_LEN, "%s", text);
        s_logger.ring_count++;
    } else {
        memmove(s_logger.ring[0], s_logger.ring[1],
                (MKL_RING_CAPACITY - 1) * MKL_LINE_LEN);
        snprintf(s_logger.ring[MKL_RING_CAPACITY - 1], MKL_LINE_LEN, "%s", text);
    }
    sink_count = s_logger.sink_count;
    memcpy(sinks, s_logger.sinks, sizeof(mkl_log_sink) * (size_t)sink_count);
    if (fatal) {
        ring_copy_count = s_logger.ring_count;
        memcpy(ring_copy, s_logger.ring, sizeof(char[MKL_LINE_LEN]) * (size_t)ring_copy_count);
        fatal_handler = s_logger.fatal_handler;
        fatal_userdata = s_logger.fatal_userdata;
    }
    mkl_mutex_unlock(&s_logger.mutex);

    for (int i = 0; i < sink_count; ++i) {
        sinks[i].fn(level, text, sinks[i].userdata);
    }
    if (fatal && fatal_handler) {
        const char* lines[MKL_RING_CAPACITY];
        for (int i = 0; i < ring_copy_count; ++i) {
            lines[i] = ring_copy[i];
        }
        fatal_handler(lines, ring_copy_count, fatal_userdata);
    }
}

void mkl_logger_flush(void) {
    if (!s_logger.initialized) {
        return;
    }
    mkl_mutex_lock(&s_logger.mutex);
    if (s_logger.file) {
        fflush(s_logger.file);
    }
    fflush(stdout);
    mkl_mutex_unlock(&s_logger.mutex);
}

void mkl_logger_subscribe(mkl_log_sink_fn sink, void* userdata) {
    mkl_mutex_lock(&s_logger.mutex);
    if (s_logger.sink_count < 16) {
        s_logger.sinks[s_logger.sink_count].fn = sink;
        s_logger.sinks[s_logger.sink_count].userdata = userdata;
        s_logger.sink_count++;
    }
    mkl_mutex_unlock(&s_logger.mutex);
}

int mkl_logger_recent_lines(char (*out)[MKL_LINE_LEN], int max_lines) {
    mkl_mutex_lock(&s_logger.mutex);
    int count = s_logger.ring_count < max_lines ? s_logger.ring_count : max_lines;
    int start = s_logger.ring_count - count;
    for (int i = 0; i < count; ++i) {
        snprintf(out[i], MKL_LINE_LEN, "%s", s_logger.ring[start + i]);
    }
    mkl_mutex_unlock(&s_logger.mutex);
    return count;
}

void mkl_logger_set_fatal_handler(mkl_fatal_handler_fn handler, void* userdata) {
    mkl_mutex_lock(&s_logger.mutex);
    s_logger.fatal_handler = handler;
    s_logger.fatal_userdata = userdata;
    mkl_mutex_unlock(&s_logger.mutex);
}
