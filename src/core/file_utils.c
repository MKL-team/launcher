#include "file_utils.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

/* ---- 平台路径分隔符 ---- */
#ifdef _WIN32
#define MKL_PATH_SEP '\\'
#define MKL_PATH_SEP_STR "\\"
#else
#define MKL_PATH_SEP '/'
#define MKL_PATH_SEP_STR "/"
#endif

int mkl_file_exists(const char* path) {
    if (!path) {
        return 0;
    }
    FILE* f = fopen(path, "rb");
    if (f) {
        fclose(f);
        return 1;
    }
    return 0;
}

int mkl_file_create_directories(const char* path) {
    if (!path || *path == '\0') {
        return -1;
    }
    char tmp[1024];
    size_t len = strlen(path);
    if (len >= sizeof(tmp)) {
        return -1;
    }
    memcpy(tmp, path, len + 1);
    for (size_t i = 0; tmp[i] != '\0'; ++i) {
        if (tmp[i] == '/' || tmp[i] == '\\') {
            char c = tmp[i];
            tmp[i] = '\0';
            if (tmp[0] != '\0') {
#ifdef _WIN32
                _mkdir(tmp);
#else
                mkdir(tmp, 0755);
#endif
            }
            tmp[i] = c;
        }
    }
#ifdef _WIN32
    _mkdir(tmp);
#else
    mkdir(tmp, 0755);
#endif
    return 0;
}

long mkl_file_read_all_text(const char* path, char* out, size_t out_sz) {
    if (!path || !out || out_sz == 0) {
        return -1;
    }
    FILE* f = fopen(path, "rb");
    if (!f) {
        return -1;
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return -1;
    }
    long size = ftell(f);
    if (size < 0) {
        fclose(f);
        return -1;
    }
    rewind(f);
    if ((size_t)size >= out_sz) {
        fclose(f);
        return -1; /* 超出调用方缓冲区 */
    }
    size_t rd = fread(out, 1, (size_t)size, f);
    fclose(f);
    out[rd] = '\0';
    return (long)rd;
}

int mkl_file_write_all_text(const char* path, const char* content) {
    if (!path || !content) {
        return -1;
    }
    FILE* f = fopen(path, "wb");
    if (!f) {
        return -1;
    }
    size_t n = strlen(content);
    size_t w = fwrite(content, 1, n, f);
    int ok = (w == n) ? 0 : -1;
    fclose(f);
    return ok;
}

int mkl_file_copy(const char* src, const char* dest) {
    FILE* in = fopen(src, "rb");
    if (!in) {
        return -1;
    }
    FILE* out = fopen(dest, "wb");
    if (!out) {
        fclose(in);
        return -1;
    }
    char buf[8192];
    size_t n;
    int ok = 0;
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
        if (fwrite(buf, 1, n, out) != n) {
            ok = -1;
            break;
        }
    }
    fclose(in);
    fclose(out);
    return ok;
}

int mkl_file_move(const char* src, const char* dest) {
    if (rename(src, dest) == 0) {
        return 0;
    }
    if (mkl_file_copy(src, dest) == 0) {
        return mkl_file_delete(src);
    }
    return -1;
}

int mkl_file_delete(const char* path) {
    return remove(path) == 0 ? 0 : -1;
}

#ifdef _WIN32
static void remove_recursive(const char* path) {
    WIN32_FIND_DATAA fd;
    char pattern[1024];
    snprintf(pattern, sizeof(pattern), "%s\\*", path);
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) {
        DeleteFileA(path);
        RemoveDirectoryA(path);
        return;
    }
    do {
        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0) {
            continue;
        }
        char child[1024];
        snprintf(child, sizeof(child), "%s\\%s", path, fd.cFileName);
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            remove_recursive(child);
        } else {
            DeleteFileA(child);
        }
    } while (FindNextFileA(h, &fd));
    FindClose(h);
    RemoveDirectoryA(path);
}
#else
static void remove_recursive(const char* path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return;
    }
    if (S_ISDIR(st.st_mode)) {
        DIR* d = opendir(path);
        if (!d) {
            return;
        }
        struct dirent* e;
        while ((e = readdir(d)) != NULL) {
            if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) {
                continue;
            }
            char child[1024];
            snprintf(child, sizeof(child), "%s%s%s", path, MKL_PATH_SEP_STR, e->d_name);
            remove_recursive(child);
        }
        closedir(d);
        rmdir(path);
    } else {
        remove(path);
    }
}
#endif

int mkl_file_delete_directory(const char* path) {
    if (!path || !mkl_file_exists(path)) {
        return 0;
    }
    remove_recursive(path);
    return 0;
}

long long mkl_file_size(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        return -1;
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return -1;
    }
    long long size = (long long)ftell(f);
    fclose(f);
    return size;
}

void mkl_file_format_size(long long bytes, char* out, size_t out_sz) {
    static const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    double value = (double)bytes;
    int unit = 0;
    while (value >= 1024.0 && unit < 4) {
        value /= 1024.0;
        ++unit;
    }
    if (unit == 0) {
        snprintf(out, out_sz, "%lld B", bytes);
    } else {
        snprintf(out, out_sz, "%.1f %s", value, units[unit]);
    }
}

/* ---- SHA-1（标准实现，C11；平台 SIMD 优化后续放入 platform/asm） ---- */

static uint32_t rol32(uint32_t x, int n) {
    return (x << n) | (x >> (32 - n));
}

int mkl_file_sha1(const char* path, char out_hex[41]) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        return -1;
    }
    /* 读取全部数据（骨架阶段；大文件流式处理后续优化） */
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return -1;
    }
    long size = ftell(f);
    if (size < 0) {
        fclose(f);
        return -1;
    }
    rewind(f);
    uint8_t* data = (uint8_t*)malloc((size_t)size > 0 ? (size_t)size : 1);
    if (!data) {
        fclose(f);
        return -1;
    }
    size_t rd = fread(data, 1, (size_t)size, f);
    fclose(f);
    if (rd != (size_t)size) {
        free(data);
        return -1;
    }

    uint64_t bitlen = (uint64_t)size * 8ULL;
    size_t msg_len = (size_t)size + 1;
    while (msg_len % 64 != 56) {
        ++msg_len;
    }
    msg_len += 8;
    uint8_t* msg = (uint8_t*)malloc(msg_len);
    if (!msg) {
        free(data);
        return -1;
    }
    memcpy(msg, data, (size_t)size);
    free(data);
    msg[(size_t)size] = 0x80;
    for (size_t i = (size_t)size + 1; i < msg_len - 8; ++i) {
        msg[i] = 0x00;
    }
    for (int i = 7; i >= 0; --i) {
        msg[msg_len - 8 + (size_t)i] = (uint8_t)((bitlen >> (i * 8)) & 0xFF);
    }

    uint32_t h0 = 0x67452301, h1 = 0xEFCDAB89, h2 = 0x98BADCFE, h3 = 0x10325476,
             h4 = 0xC3D2E1F0;

    for (size_t i = 0; i + 64 <= msg_len; i += 64) {
        uint32_t w[80];
        for (int j = 0; j < 16; ++j) {
            w[j] = ((uint32_t)msg[i + (size_t)j * 4] << 24) |
                   ((uint32_t)msg[i + (size_t)j * 4 + 1] << 16) |
                   ((uint32_t)msg[i + (size_t)j * 4 + 2] << 8) |
                   (uint32_t)msg[i + (size_t)j * 4 + 3];
        }
        for (int j = 16; j < 80; ++j) {
            w[j] = rol32(w[j - 3] ^ w[j - 8] ^ w[j - 14] ^ w[j - 16], 1);
        }
        uint32_t a = h0, b = h1, c = h2, d = h3, e = h4;
        for (int j = 0; j < 80; ++j) {
            uint32_t f, k;
            if (j < 20) {
                f = (b & c) | (~b & d);
                k = 0x5A827999;
            } else if (j < 40) {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1;
            } else if (j < 60) {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDC;
            } else {
                f = b ^ c ^ d;
                k = 0xCA62C1D6;
            }
            uint32_t temp = rol32(a, 5) + f + e + k + w[j];
            e = d;
            d = c;
            c = rol32(b, 30);
            b = a;
            a = temp;
        }
        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
        h4 += e;
    }
    free(msg);

    snprintf(out_hex, 41, "%08x%08x%08x%08x%08x", h0, h1, h2, h3, h4);
    return 0;
}

void mkl_file_temp_directory(char* out, size_t out_sz) {
    const char* p = getenv("TMPDIR");
    if (!p) {
        p = getenv("TMP");
    }
    if (!p) {
        p = getenv("TEMP");
    }
    if (!p) {
#ifdef _WIN32
        p = "C:\\Windows\\Temp";
#else
        p = "/tmp";
#endif
    }
    snprintf(out, out_sz, "%s", p);
}

void mkl_file_appdata_directory(char* out, size_t out_sz) {
#if defined(_WIN32)
    const char* p = getenv("APPDATA");
    if (p && *p) {
        snprintf(out, out_sz, "%s", p);
        return;
    }
#elif defined(__APPLE__)
    const char* home = getenv("HOME");
    if (home && *home) {
        snprintf(out, out_sz, "%s/Library/Application Support", home);
        return;
    }
#else
    const char* xdg = getenv("XDG_CONFIG_HOME");
    if (xdg && *xdg) {
        snprintf(out, out_sz, "%s", xdg);
        return;
    }
    const char* home = getenv("HOME");
    if (home && *home) {
        snprintf(out, out_sz, "%s/.config", home);
        return;
    }
#endif
    mkl_file_temp_directory(out, out_sz);
}

void mkl_file_join_path(const char* const* parts, int count, char* out, size_t out_sz) {
    if (!out || out_sz == 0) {
        return;
    }
    out[0] = '\0';
    if (!parts || count <= 0) {
        return;
    }
    size_t pos = 0;
    for (int i = 0; i < count; ++i) {
        const char* part = parts[i] ? parts[i] : "";
        size_t need = strlen(part) + (i > 0 ? 1 : 0);
        if (pos + need + 1 > out_sz) {
            break;
        }
        if (i > 0) {
            out[pos++] = MKL_PATH_SEP;
        }
        memcpy(out + pos, part, strlen(part));
        pos += strlen(part);
    }
    out[pos] = '\0';
}
