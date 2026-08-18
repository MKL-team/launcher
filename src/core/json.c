#include "json.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- 内部节点树 ---- */
enum {
    MKL_JN_NULL = 0,
    MKL_JN_BOOL,
    MKL_JN_NUMBER,
    MKL_JN_STRING,
    MKL_JN_ARRAY,
    MKL_JN_OBJECT
};

typedef struct mkl_json_kv {
    char* key;
    struct mkl_json_node* val; /* 堆上节点 */
} mkl_json_kv;

typedef struct mkl_json_node {
    int type;
    int bval;
    double num;
    char* str;
    struct mkl_json_node* arr; /* 数组元素（堆上连续） */
    int arr_count;
    mkl_json_kv* obj; /* 对象键值对（堆上连续） */
    int obj_count;
} mkl_json_node;

typedef struct mkl_json_parser {
    const char* s;
    size_t pos;
    size_t len;
} mkl_json_parser;

static void mkl_json_node_free(mkl_json_node* n) {
    if (!n) {
        return;
    }
    if (n->str) {
        free(n->str);
        n->str = NULL;
    }
    if (n->arr) {
        for (int i = 0; i < n->arr_count; ++i) {
            mkl_json_node_free(&n->arr[i]);
        }
        free(n->arr);
        n->arr = NULL;
    }
    if (n->obj) {
        for (int i = 0; i < n->obj_count; ++i) {
            free(n->obj[i].key);
            mkl_json_node_free(n->obj[i].val);
            free(n->obj[i].val);
        }
        free(n->obj);
        n->obj = NULL;
    }
    n->arr_count = 0;
    n->obj_count = 0;
}

static void skip_ws(mkl_json_parser* p) {
    while (p->pos < p->len && isspace((unsigned char)p->s[p->pos])) {
        ++p->pos;
    }
}

static void append_utf8(char** buf, size_t* used, size_t* cap, unsigned int cp) {
    if (cp < 0x80) {
        if (*used + 1 >= *cap) {
            *cap = *cap * 2 + 16;
            *buf = (char*)realloc(*buf, *cap);
        }
        (*buf)[(*used)++] = (char)cp;
    } else if (cp < 0x800) {
        if (*used + 2 >= *cap) {
            *cap = *cap * 2 + 16;
            *buf = (char*)realloc(*buf, *cap);
        }
        (*buf)[(*used)++] = (char)(0xC0 | (cp >> 6));
        (*buf)[(*used)++] = (char)(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        if (*used + 3 >= *cap) {
            *cap = *cap * 2 + 16;
            *buf = (char*)realloc(*buf, *cap);
        }
        (*buf)[(*used)++] = (char)(0xE0 | (cp >> 12));
        (*buf)[(*used)++] = (char)(0x80 | ((cp >> 6) & 0x3F));
        (*buf)[(*used)++] = (char)(0x80 | (cp & 0x3F));
    } else {
        if (*used + 4 >= *cap) {
            *cap = *cap * 2 + 16;
            *buf = (char*)realloc(*buf, *cap);
        }
        (*buf)[(*used)++] = (char)(0xF0 | (cp >> 18));
        (*buf)[(*used)++] = (char)(0x80 | ((cp >> 12) & 0x3F));
        (*buf)[(*used)++] = (char)(0x80 | ((cp >> 6) & 0x3F));
        (*buf)[(*used)++] = (char)(0x80 | (cp & 0x3F));
    }
}

/* 解析字符串（含转义），返回堆上字符串；失败返回 NULL */
static char* parse_string(mkl_json_parser* p) {
    if (p->pos >= p->len || p->s[p->pos] != '"') {
        return NULL;
    }
    ++p->pos;
    size_t cap = 32, used = 0;
    char* out = (char*)malloc(cap);
    if (!out) {
        return NULL;
    }
    while (p->pos < p->len) {
        char c = p->s[p->pos++];
        if (c == '"') {
            out[used] = '\0';
            return out;
        }
        if (c != '\\') {
            if (used + 2 >= cap) {
                cap *= 2;
                out = (char*)realloc(out, cap);
            }
            out[used++] = c;
            continue;
        }
        if (p->pos >= p->len) {
            free(out);
            return NULL;
        }
        char e = p->s[p->pos++];
        switch (e) {
            case '"': out[used++] = '"'; break;
            case '\\': out[used++] = '\\'; break;
            case '/': out[used++] = '/'; break;
            case 'b': out[used++] = '\b'; break;
            case 'f': out[used++] = '\f'; break;
            case 'n': out[used++] = '\n'; break;
            case 'r': out[used++] = '\r'; break;
            case 't': out[used++] = '\t'; break;
            case 'u': {
                if (p->pos + 4 > p->len) {
                    free(out);
                    return NULL;
                }
                unsigned int cp = 0;
                for (int k = 0; k < 4; ++k) {
                    char h = p->s[p->pos++];
                    cp <<= 4;
                    if (h >= '0' && h <= '9') {
                        cp |= (unsigned int)(h - '0');
                    } else if (h >= 'a' && h <= 'f') {
                        cp |= (unsigned int)(h - 'a' + 10);
                    } else if (h >= 'A' && h <= 'F') {
                        cp |= (unsigned int)(h - 'A' + 10);
                    } else {
                        free(out);
                        return NULL;
                    }
                }
                append_utf8(&out, &used, &cap, cp);
                break;
            }
            default:
                free(out);
                return NULL;
        }
    }
    free(out);
    return NULL; /* 未闭合 */
}

static int parse_number(mkl_json_parser* p, mkl_json_node* out) {
    size_t start = p->pos;
    if (p->pos < p->len && p->s[p->pos] == '-') {
        ++p->pos;
    }
    int has_digit = 0;
    while (p->pos < p->len && isdigit((unsigned char)p->s[p->pos])) {
        ++p->pos;
        has_digit = 1;
    }
    if (p->pos < p->len && p->s[p->pos] == '.') {
        ++p->pos;
        while (p->pos < p->len && isdigit((unsigned char)p->s[p->pos])) {
            ++p->pos;
            has_digit = 1;
        }
    }
    if (p->pos < p->len && (p->s[p->pos] == 'e' || p->s[p->pos] == 'E')) {
        ++p->pos;
        if (p->pos < p->len && (p->s[p->pos] == '+' || p->s[p->pos] == '-')) {
            ++p->pos;
        }
        while (p->pos < p->len && isdigit((unsigned char)p->s[p->pos])) {
            ++p->pos;
        }
    }
    if (!has_digit) {
        return 0;
    }
    char tmp[64];
    size_t n = p->pos - start;
    if (n >= sizeof(tmp)) {
        n = sizeof(tmp) - 1;
    }
    memcpy(tmp, p->s + start, n);
    tmp[n] = '\0';
    out->type = MKL_JN_NUMBER;
    out->num = strtod(tmp, NULL);
    return 1;
}

static int parse_literal(mkl_json_parser* p, const char* lit, mkl_json_node* out, int type,
                         int bval) {
    size_t n = strlen(lit);
    if (p->pos + n > p->len || strncmp(p->s + p->pos, lit, n) != 0) {
        return 0;
    }
    p->pos += n;
    out->type = type;
    out->bval = bval;
    return 1;
}

static int parse_value(mkl_json_parser* p, mkl_json_node* out);

static int parse_array(mkl_json_parser* p, mkl_json_node* out) {
    ++p->pos; /* '[' */
    out->type = MKL_JN_ARRAY;
    out->arr = NULL;
    out->arr_count = 0;
    skip_ws(p);
    if (p->pos < p->len && p->s[p->pos] == ']') {
        ++p->pos;
        return 1;
    }
    while (1) {
        skip_ws(p);
        mkl_json_node v;
        memset(&v, 0, sizeof(v));
        if (!parse_value(p, &v)) {
            return 0;
        }
        mkl_json_node* grown =
            (mkl_json_node*)realloc(out->arr, sizeof(mkl_json_node) * (size_t)(out->arr_count + 1));
        if (!grown) {
            mkl_json_node_free(&v);
            return 0;
        }
        out->arr = grown;
        out->arr[out->arr_count] = v;
        out->arr_count++;
        skip_ws(p);
        if (p->pos >= p->len) {
            return 0;
        }
        if (p->s[p->pos] == ',') {
            ++p->pos;
            continue;
        }
        if (p->s[p->pos] == ']') {
            ++p->pos;
            return 1;
        }
        return 0;
    }
}

static int parse_object(mkl_json_parser* p, mkl_json_node* out) {
    ++p->pos; /* '{' */
    out->type = MKL_JN_OBJECT;
    out->obj = NULL;
    out->obj_count = 0;
    skip_ws(p);
    if (p->pos < p->len && p->s[p->pos] == '}') {
        ++p->pos;
        return 1;
    }
    while (1) {
        skip_ws(p);
        char* key = parse_string(p);
        if (!key) {
            return 0;
        }
        skip_ws(p);
        if (p->pos >= p->len || p->s[p->pos] != ':') {
            free(key);
            return 0;
        }
        ++p->pos;
        skip_ws(p);
        mkl_json_node* v = (mkl_json_node*)malloc(sizeof(mkl_json_node));
        if (!v) {
            free(key);
            return 0;
        }
        memset(v, 0, sizeof(*v));
        if (!parse_value(p, v)) {
            free(key);
            mkl_json_node_free(v);
            free(v);
            return 0;
        }
        mkl_json_kv* grown =
            (mkl_json_kv*)realloc(out->obj, sizeof(mkl_json_kv) * (size_t)(out->obj_count + 1));
        if (!grown) {
            free(key);
            mkl_json_node_free(v);
            free(v);
            return 0;
        }
        out->obj = grown;
        out->obj[out->obj_count].key = key;
        out->obj[out->obj_count].val = v;
        out->obj_count++;
        skip_ws(p);
        if (p->pos >= p->len) {
            return 0;
        }
        if (p->s[p->pos] == ',') {
            ++p->pos;
            continue;
        }
        if (p->s[p->pos] == '}') {
            ++p->pos;
            return 1;
        }
        return 0;
    }
}

static int parse_value(mkl_json_parser* p, mkl_json_node* out) {
    skip_ws(p);
    if (p->pos >= p->len) {
        return 0;
    }
    char c = p->s[p->pos];
    if (c == '{') {
        return parse_object(p, out);
    }
    if (c == '[') {
        return parse_array(p, out);
    }
    if (c == '"') {
        char* s = parse_string(p);
        if (!s) {
            return 0;
        }
        out->type = MKL_JN_STRING;
        out->str = s;
        return 1;
    }
    if (c == 't') {
        return parse_literal(p, "true", out, MKL_JN_BOOL, 1);
    }
    if (c == 'f') {
        return parse_literal(p, "false", out, MKL_JN_BOOL, 0);
    }
    if (c == 'n') {
        return parse_literal(p, "null", out, MKL_JN_NULL, 0);
    }
    return parse_number(p, out);
}

/* 解析整棵树；成功返回节点（调用方负责 mkl_json_node_free），失败返回 NULL */
static mkl_json_node* mkl_json_parse(const char* s) {
    if (!s) {
        return NULL;
    }
    mkl_json_parser p;
    p.s = s;
    p.pos = 0;
    p.len = strlen(s);
    mkl_json_node* root = (mkl_json_node*)malloc(sizeof(mkl_json_node));
    if (!root) {
        return NULL;
    }
    memset(root, 0, sizeof(*root));
    if (!parse_value(&p, root)) {
        mkl_json_node_free(root);
        free(root);
        return NULL;
    }
    skip_ws(&p);
    if (p.pos != p.len) {
        mkl_json_node_free(root);
        free(root);
        return NULL;
    }
    return root;
}

/* ---- 路径查询 ---- */
#define MKL_PATH_MAX_SEGS 8

static int split_path(const char* key_path, char segs[][MKL_JSON_KEY_LEN]) {
    if (!key_path || *key_path == '\0') {
        return 0;
    }
    int count = 0;
    const char* start = key_path;
    for (const char* p = key_path;; ++p) {
        if (*p == '.' || *p == '\0') {
            size_t len = (size_t)(p - start);
            if (len >= MKL_JSON_KEY_LEN) {
                len = MKL_JSON_KEY_LEN - 1;
            }
            if (count < MKL_PATH_MAX_SEGS) {
                memcpy(segs[count], start, len);
                segs[count][len] = '\0';
                count++;
            }
            start = p + 1;
            if (*p == '\0') {
                break;
            }
        }
    }
    return count;
}

static mkl_json_node* find_path(mkl_json_node* root, char segs[][MKL_JSON_KEY_LEN], int count) {
    mkl_json_node* cur = root;
    for (int i = 0; i < count; ++i) {
        if (!cur) {
            return NULL;
        }
        if (cur->type == MKL_JN_OBJECT) {
            mkl_json_node* next = NULL;
            for (int j = 0; j < cur->obj_count; ++j) {
                if (strcmp(cur->obj[j].key, segs[i]) == 0) {
                    next = cur->obj[j].val;
                    break;
                }
            }
            cur = next;
        } else if (cur->type == MKL_JN_ARRAY) {
            int is_num = 1;
            if (segs[i][0] == '\0') {
                is_num = 0;
            }
            for (const char* q = segs[i]; *q; ++q) {
                if (!isdigit((unsigned char)*q)) {
                    is_num = 0;
                    break;
                }
            }
            if (!is_num) {
                return NULL;
            }
            int idx = atoi(segs[i]);
            if (idx < 0 || idx >= cur->arr_count) {
                return NULL;
            }
            cur = &cur->arr[idx];
        } else {
            return NULL;
        }
    }
    return cur;
}

/* ---- 序列化辅助 ---- */
static void escape_string(const char* s, char* out, size_t out_sz) {
    size_t o = 0;
    if (o + 1 < out_sz) {
        out[o++] = '"';
    }
    for (size_t i = 0; s[i] != '\0' && o + 2 < out_sz; ++i) {
        switch (s[i]) {
            case '"': out[o++] = '\\'; out[o++] = '"'; break;
            case '\\': out[o++] = '\\'; out[o++] = '\\'; break;
            case '\n': out[o++] = '\\'; out[o++] = 'n'; break;
            case '\r': out[o++] = '\\'; out[o++] = 'r'; break;
            case '\t': out[o++] = '\\'; out[o++] = 't'; break;
            default: out[o++] = s[i]; break;
        }
    }
    if (o + 1 < out_sz) {
        out[o++] = '"';
    }
    out[o] = '\0';
}

/* ---- 公共接口 ---- */

int mkl_json_is_valid(const char* s) {
    mkl_json_node* root = mkl_json_parse(s);
    if (!root) {
        return 0;
    }
    mkl_json_node_free(root);
    free(root);
    return 1;
}

void mkl_json_get_string(const char* json, const char* key_path, char* out, size_t out_sz,
                         const char* def) {
    if (!out || out_sz == 0) {
        return;
    }
    mkl_json_node* root = mkl_json_parse(json);
    if (root) {
        char segs[MKL_PATH_MAX_SEGS][MKL_JSON_KEY_LEN];
        int count = split_path(key_path, segs);
        mkl_json_node* n = find_path(root, segs, count);
        if (n && n->type == MKL_JN_STRING && n->str) {
            snprintf(out, out_sz, "%s", n->str);
            mkl_json_node_free(root);
            free(root);
            return;
        }
        mkl_json_node_free(root);
        free(root);
    }
    snprintf(out, out_sz, "%s", def ? def : "");
}

long long mkl_json_get_int(const char* json, const char* key_path, long long def) {
    mkl_json_node* root = mkl_json_parse(json);
    if (!root) {
        return def;
    }
    char segs[MKL_PATH_MAX_SEGS][MKL_JSON_KEY_LEN];
    int count = split_path(key_path, segs);
    mkl_json_node* n = find_path(root, segs, count);
    long long result = def;
    if (n) {
        if (n->type == MKL_JN_NUMBER) {
            result = (long long)n->num;
        } else if (n->type == MKL_JN_STRING && n->str) {
            result = strtoll(n->str, NULL, 10);
        }
    }
    mkl_json_node_free(root);
    free(root);
    return result;
}

int mkl_json_get_bool(const char* json, const char* key_path, int def) {
    mkl_json_node* root = mkl_json_parse(json);
    if (!root) {
        return def;
    }
    char segs[MKL_PATH_MAX_SEGS][MKL_JSON_KEY_LEN];
    int count = split_path(key_path, segs);
    mkl_json_node* n = find_path(root, segs, count);
    int result = def;
    if (n && n->type == MKL_JN_BOOL) {
        result = n->bval;
    }
    mkl_json_node_free(root);
    free(root);
    return result;
}

int mkl_json_get_string_array(const char* json, const char* key_path, char out[][MKL_JSON_ITEM_LEN],
                              int max_items) {
    if (!out || max_items <= 0) {
        return 0;
    }
    mkl_json_node* root = mkl_json_parse(json);
    if (!root) {
        return 0;
    }
    char segs[MKL_PATH_MAX_SEGS][MKL_JSON_KEY_LEN];
    int count = split_path(key_path, segs);
    mkl_json_node* n = find_path(root, segs, count);
    int result = 0;
    if (n && n->type == MKL_JN_ARRAY) {
        for (int i = 0; i < n->arr_count && result < max_items; ++i) {
            if (n->arr[i].type == MKL_JN_STRING && n->arr[i].str) {
                snprintf(out[result], MKL_JSON_ITEM_LEN, "%s", n->arr[i].str);
                result++;
            }
        }
    }
    mkl_json_node_free(root);
    free(root);
    return result;
}

int mkl_json_get_string_map(const char* json, const char* key_path, char keys[][MKL_JSON_KEY_LEN],
                            char vals[][MKL_JSON_ITEM_LEN], int max_pairs) {
    if (!keys || !vals || max_pairs <= 0) {
        return 0;
    }
    mkl_json_node* root = mkl_json_parse(json);
    if (!root) {
        return 0;
    }
    char segs[MKL_PATH_MAX_SEGS][MKL_JSON_KEY_LEN];
    int count = split_path(key_path, segs);
    mkl_json_node* n = find_path(root, segs, count);
    int result = 0;
    if (n && n->type == MKL_JN_OBJECT) {
        for (int i = 0; i < n->obj_count && result < max_pairs; ++i) {
            if (n->obj[i].val && n->obj[i].val->type == MKL_JN_STRING &&
                n->obj[i].val->str) {
                snprintf(keys[result], MKL_JSON_KEY_LEN, "%s", n->obj[i].key);
                snprintf(vals[result], MKL_JSON_ITEM_LEN, "%s", n->obj[i].val->str);
                result++;
            }
        }
    }
    mkl_json_node_free(root);
    free(root);
    return result;
}

int mkl_json_serialize_object(const char* const keys[], const char* const vals[], int count,
                              char* out, size_t out_sz) {
    if (!out || out_sz == 0) {
        return -1;
    }
    size_t pos = 0;
    if (pos < out_sz) {
        out[pos++] = '{';
    }
    for (int i = 0; i < count; ++i) {
        if (i > 0 && pos < out_sz) {
            out[pos++] = ',';
        }
        char k[MKL_JSON_KEY_LEN * 2];
        char v[MKL_JSON_ITEM_LEN * 2];
        escape_string(keys ? (keys[i] ? keys[i] : "") : "", k, sizeof(k));
        escape_string(vals ? (vals[i] ? vals[i] : "") : "", v, sizeof(v));
        size_t klen = strlen(k), vlen = strlen(v);
        if (pos + klen + 1 + vlen + 1 >= out_sz) {
            break;
        }
        memcpy(out + pos, k, klen);
        pos += klen;
        out[pos++] = ':';
        memcpy(out + pos, v, vlen);
        pos += vlen;
    }
    if (pos < out_sz) {
        out[pos++] = '}';
    }
    out[pos < out_sz ? pos : out_sz - 1] = '\0';
    return 0;
}

int mkl_json_serialize_string_array(const char* const items[], int count, char* out,
                                    size_t out_sz) {
    if (!out || out_sz == 0) {
        return -1;
    }
    size_t pos = 0;
    if (pos < out_sz) {
        out[pos++] = '[';
    }
    for (int i = 0; i < count; ++i) {
        if (i > 0 && pos < out_sz) {
            out[pos++] = ',';
        }
        char v[MKL_JSON_ITEM_LEN * 2];
        escape_string(items ? (items[i] ? items[i] : "") : "", v, sizeof(v));
        size_t vlen = strlen(v);
        if (pos + vlen + 1 >= out_sz) {
            break;
        }
        memcpy(out + pos, v, vlen);
        pos += vlen;
    }
    if (pos < out_sz) {
        out[pos++] = ']';
    }
    out[pos < out_sz ? pos : out_sz - 1] = '\0';
    return 0;
}
