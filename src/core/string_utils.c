#include "string_utils.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

void mkl_string_trim(const char* s, char* out, size_t out_sz) {
    if (!s || !out || out_sz == 0) {
        if (out && out_sz > 0) {
            out[0] = '\0';
        }
        return;
    }
    size_t start = 0;
    size_t end = strlen(s);
    while (start < end && isspace((unsigned char)s[start])) {
        ++start;
    }
    while (end > start && isspace((unsigned char)s[end - 1])) {
        --end;
    }
    size_t len = end - start;
    if (len >= out_sz) {
        len = out_sz - 1;
    }
    memcpy(out, s + start, len);
    out[len] = '\0';
}

int mkl_string_split(const char* s, char delimiter, char parts[][MKL_STRING_PART_LEN],
                     int max_parts) {
    if (!s || !parts || max_parts <= 0) {
        return 0;
    }
    int count = 0;
    const char* start = s;
    for (const char* p = s;; ++p) {
        if (*p == delimiter || *p == '\0') {
            size_t len = (size_t)(p - start);
            if (len >= MKL_STRING_PART_LEN) {
                len = MKL_STRING_PART_LEN - 1;
            }
            if (count < max_parts) {
                memcpy(parts[count], start, len);
                parts[count][len] = '\0';
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

int mkl_string_replace(const char* s, const char* from, const char* to, char* out,
                       size_t out_sz) {
    if (!s || !from || !to || !out || out_sz == 0) {
        if (out && out_sz > 0) {
            out[0] = '\0';
        }
        return 0;
    }
    if (*from == '\0') {
        snprintf(out, out_sz, "%s", s);
        return 0;
    }
    size_t from_len = strlen(from);
    size_t to_len = strlen(to);
    size_t pos = 0;
    size_t opos = 0;
    int count = 0;
    while (s[pos] != '\0') {
        const char* found = strstr(s + pos, from);
        if (!found) {
            break;
        }
        size_t prefix = (size_t)(found - (s + pos));
        if (opos + prefix >= out_sz - 1) {
            break;
        }
        memcpy(out + opos, s + pos, prefix);
        opos += prefix;
        if (opos + to_len >= out_sz - 1) {
            break;
        }
        memcpy(out + opos, to, to_len);
        opos += to_len;
        pos += prefix + from_len;
        count++;
    }
    size_t rest = strlen(s + pos);
    if (opos + rest < out_sz) {
        memcpy(out + opos, s + pos, rest + 1);
    } else if (out_sz > 0) {
        out[out_sz - 1] = '\0';
    }
    return count;
}

int mkl_string_startswith(const char* s, const char* prefix) {
    if (!s || !prefix) {
        return 0;
    }
    size_t n = strlen(prefix);
    return strncmp(s, prefix, n) == 0;
}

int mkl_string_endswith(const char* s, const char* suffix) {
    if (!s || !suffix) {
        return 0;
    }
    size_t sl = strlen(s);
    size_t nl = strlen(suffix);
    if (nl > sl) {
        return 0;
    }
    return strcmp(s + sl - nl, suffix) == 0;
}

void mkl_string_tolower(const char* s, char* out, size_t out_sz) {
    if (!out || out_sz == 0) {
        return;
    }
    size_t i = 0;
    for (; s[i] != '\0' && i + 1 < out_sz; ++i) {
        out[i] = (char)tolower((unsigned char)s[i]);
    }
    out[i] = '\0';
}

void mkl_string_toupper(const char* s, char* out, size_t out_sz) {
    if (!out || out_sz == 0) {
        return;
    }
    size_t i = 0;
    for (; s[i] != '\0' && i + 1 < out_sz; ++i) {
        out[i] = (char)toupper((unsigned char)s[i]);
    }
    out[i] = '\0';
}

int mkl_string_format(char* out, size_t out_sz, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(out, out_sz, fmt, args);
    va_end(args);
    return n;
}

int mkl_string_parse_query(const char* qs, char keys[][MKL_STRING_KEY_LEN],
                           char vals[][MKL_STRING_PART_LEN], int max_pairs) {
    if (!qs || !keys || !vals || max_pairs <= 0) {
        return 0;
    }
    const char* body = qs;
    if (*body == '?') {
        body++;
    }
    char pairs[MKL_STRING_MAX_PARTS][MKL_STRING_PART_LEN];
    int n = mkl_string_split(body, '&', pairs, MKL_STRING_MAX_PARTS);
    int count = 0;
    for (int i = 0; i < n && count < max_pairs; ++i) {
        char* eq = strchr(pairs[i], '=');
        if (eq) {
            *eq = '\0';
            snprintf(keys[count], MKL_STRING_KEY_LEN, "%s", pairs[i]);
            snprintf(vals[count], MKL_STRING_PART_LEN, "%s", eq + 1);
        } else {
            snprintf(keys[count], MKL_STRING_KEY_LEN, "%s", pairs[i]);
            vals[count][0] = '\0';
        }
        count++;
    }
    return count;
}
