#ifndef MKL_FILE_UTILS_H
#define MKL_FILE_UTILS_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int mkl_file_exists(const char* path);            /* 1=存在 0=不存在 */
int mkl_file_create_directories(const char* path);/* 0 成功 */
/* 读全部文本到 out（out_sz 含 '\0'）；返回字节数，失败或超限返回 -1 */
long mkl_file_read_all_text(const char* path, char* out, size_t out_sz);
int mkl_file_write_all_text(const char* path, const char* content);
int mkl_file_copy(const char* src, const char* dest);
int mkl_file_move(const char* src, const char* dest);
int mkl_file_delete(const char* path);
int mkl_file_delete_directory(const char* path);  /* 递归删除 */
long long mkl_file_size(const char* path);        /* -1 失败 */
void mkl_file_format_size(long long bytes, char* out, size_t out_sz);
int mkl_file_sha1(const char* path, char out_hex[41]); /* 40 位十六进制 + '\0' */
void mkl_file_temp_directory(char* out, size_t out_sz);
void mkl_file_appdata_directory(char* out, size_t out_sz); /* 跨平台用户目录 */
void mkl_file_join_path(const char* const* parts, int count, char* out, size_t out_sz);

#ifdef __cplusplus
}
#endif

#endif /* MKL_FILE_UTILS_H */
