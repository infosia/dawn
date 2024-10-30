#include <functional>
#include <optional>
#include <ostream>
#include <queue>
#include <thread>
#include <vector>

#include "gtest/gtest.h"


class WebGPUWrapperTestSuite : public ::testing::Test {
 protected:
  // You can implement all of the common members here, or you can
  // define them in each test file.
};

TEST_F(WebGPUWrapperTestSuite, SampleTest) {
    EXPECT_EQ(4, 2+2);
}
