#ifndef MKL_TEST_UTIL_H
#define MKL_TEST_UTIL_H

#include <stdio.h>
#include <string.h>

/* 零依赖迷你测试框架（C11） */
extern int mkl_test_failures;

#define MKL_TEST(name) int mkl_test_##name(void)

#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            mkl_test_failures++;                                                 \
            fprintf(stderr, "  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);    \
        }                                                                        \
    } while (0)

#define CHECK_EQ_INT(a, b)                                                       \
    do {                                                                         \
        long long va_ = (long long)(a), vb_ = (long long)(b);                    \
        if (va_ != vb_) {                                                        \
            mkl_test_failures++;                                                 \
            fprintf(stderr, "  FAIL %s:%d: %s == %s (got %lld vs %lld)\n",       \
                    __FILE__, __LINE__, #a, #b, va_, vb_);                       \
        }                                                                        \
    } while (0)

#define CHECK_EQ_STR(a, b)                                                       \
    do {                                                                         \
        const char* va_ = (a);                                                   \
        const char* vb_ = (b);                                                   \
        if (!va_ || !vb_ || strcmp(va_, vb_) != 0) {                             \
            mkl_test_failures++;                                                 \
            fprintf(stderr, "  FAIL %s:%d: %s == %s (got \"%s\" vs \"%s\")\n",   \
                    __FILE__, __LINE__, #a, #b, va_ ? va_ : "(null)",            \
                    vb_ ? vb_ : "(null)");                                       \
        }                                                                        \
    } while (0)

#endif /* MKL_TEST_UTIL_H */
