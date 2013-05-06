#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <phantom/module.H>

namespace phantom {

MODULE(test_gtests);

TEST(Simple, FooBar) {
    ASSERT_EQ(0, 0);
}

} // namespace phantom
