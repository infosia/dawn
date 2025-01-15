#include <functional>
#include <optional>
#include <ostream>
#include <queue>
#include <thread>
#include <vector>

#include "gtest/gtest.h"

/**
 * The test suite class for the WebGPU wrapper native code.
 *
 * The unit tests are implemented using the GTest framework
 * and called via Android Instrumented Test mechanism from gradle.
 */
class WebGPUWrapperTestSuite : public ::testing::Test {
  protected:
};
