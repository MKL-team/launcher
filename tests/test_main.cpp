#include <iostream>

#include "test_util.h"

int main() {
    int failedTests = 0;
    for (const auto& t : mkl_test::registry()) {
        const int before = mkl_test::failures;
        bool ok = t.fn();
        const int after = mkl_test::failures;
        if (!ok || after != before) {
            std::cout << "[FAIL] " << t.name << "\n";
            ++failedTests;
        } else {
            std::cout << "[ OK ] " << t.name << "\n";
        }
    }
    std::cout << (failedTests == 0 && mkl_test::failures == 0 ? "ALL TESTS PASSED"
                                                               : "TESTS FAILED")
              << " (assertion failures=" << mkl_test::failures << ")\n";
    return (failedTests == 0 && mkl_test::failures == 0) ? 0 : 1;
}
