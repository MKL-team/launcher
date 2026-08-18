#pragma once

#include <functional>
#include <iostream>
#include <string>
#include <vector>

// 零依赖迷你测试框架
namespace mkl_test {

struct TestCase {
    const char* name;
    std::function<bool()> fn;
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> r;
    return r;
}

struct Registrar {
    Registrar(const char* name, std::function<bool()> fn) {
        registry().push_back({name, std::move(fn)});
    }
};

inline int failures = 0;

} // namespace mkl_test

#define MKL_TEST(name)                                                    \
    static bool mkl_test_fn_##name();                                     \
    static ::mkl_test::Registrar mkl_test_reg_##name(#name, mkl_test_fn_##name); \
    static bool mkl_test_fn_##name()

#define CHECK(cond)                                                        \
    do {                                                                   \
        if (!(cond)) {                                                     \
            ::mkl_test::failures++;                                        \
            std::cerr << "  FAIL " << __FILE__ << ":" << __LINE__          \
                      << ": " #cond << std::endl;                          \
        }                                                                  \
    } while (0)

#define CHECK_EQ(a, b)                                                     \
    do {                                                                   \
        const auto va = (a);                                               \
        const auto vb = (b);                                               \
        if (!(va == vb)) {                                                 \
            ::mkl_test::failures++;                                        \
            std::cerr << "  FAIL " << __FILE__ << ":" << __LINE__          \
                      << ": " #a " == " #b " (got: " << va << " vs "       \
                      << vb << ")" << std::endl;                           \
        }                                                                  \
    } while (0)
