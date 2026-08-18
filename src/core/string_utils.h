#ifndef MKL_STRING_UTILS_H
#define MKL_STRING_UTILS_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 段/键值对固定上限（调用方缓冲区大小约束） */
#define MKL_STRING_MAX_PARTS 64
#define MKL_STRING_PART_LEN 128
#define MKL_STRING_KEY_LEN 64

/* 去除首尾空白，写入 out（out_sz 含 '\0'） */
void mkl_string_trim(const char* s, char* out, size_t out_sz);

/* 按分隔符切分；每段写入 parts[i]（最多 max_parts 段，每段上限 MKL_STRING_PART_LEN-1）
 * 返回段数 */
int mkl_string_split(const char* s, char delimiter, char parts[][MKL_STRING_PART_LEN],
                     int max_parts);

/* 全部替换 from -> to，写入 out；返回替换次数 */
int mkl_string_replace(const char* s, const char* from, const char* to, char* out,
                       size_t out_sz);

int mkl_string_startswith(const char* s, const char* prefix);
int mkl_string_endswith(const char* s, const char* suffix);

void mkl_string_tolower(const char* s, char* out, size_t out_sz);
void mkl_string_toupper(const char* s, char* out, size_t out_sz);

/* snprintf 风格格式化 */
int mkl_string_format(char* out, size_t out_sz, const char* fmt, ...);

/* 解析查询串 "a=1&b=2"；返回对数（键/值各写入调用方数组，最多 max_pairs） */
int mkl_string_parse_query(const char* qs, char keys[][MKL_STRING_KEY_LEN],
                           char vals[][MKL_STRING_PART_LEN], int max_pairs);

#ifdef __cplusplus
}
#endif

#endif /* MKL_STRING_UTILS_H */
