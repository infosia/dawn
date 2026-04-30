#include <iostream>
#define DAWN_ASSERT_LOOP_CONDITION (0)
#define DAWN_ASSERT_CALLSITE_HELPER(condition)     do {                                                                 [[maybe_unused]] auto unused = sizeof(!!(condition));        } while (DAWN_ASSERT_LOOP_CONDITION)

int main() {
    int x = 5;
    // x is only used in DAWN_ASSERT_CALLSITE_HELPER
    DAWN_ASSERT_CALLSITE_HELPER(x == 5);
    return 0;
}
