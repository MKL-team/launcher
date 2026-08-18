#ifndef MKL_JSON_H
#define MKL_JSON_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MKL_JSON_MAX_ITEMS 64
#define MKL_JSON_ITEM_LEN 128
#define MKL_JSON_KEY_LEN 64

/* 校验是否为合法 JSON */
int mkl_json_is_valid(const char* s);

/* key_path 用点号分隔，如 "version.id"、"nested.arr.0.name" */
void mkl_json_get_string(const char* json, const char* key_path, char* out, size_t out_sz,
                         const char* def);
long long mkl_json_get_int(const char* json, const char* key_path, long long def);
int mkl_json_get_bool(const char* json, const char* key_path, int def);
/* 返回元素个数（字符串数组） */
int mkl_json_get_string_array(const char* json, const char* key_path,
                              char out[][MKL_JSON_ITEM_LEN], int max_items);
/* 返回键值对数（字符串对象） */
int mkl_json_get_string_map(const char* json, const char* key_path,
                            char keys[][MKL_JSON_KEY_LEN], char vals[][MKL_JSON_ITEM_LEN],
                            int max_pairs);

/* 序列化（键/值为字符串） */
int mkl_json_serialize_object(const char* const keys[], const char* const vals[], int count,
                              char* out, size_t out_sz);
int mkl_json_serialize_string_array(const char* const items[], int count, char* out,
                                    size_t out_sz);

#ifdef __cplusplus
}
#endif

#endif /* MKL_JSON_H */
