#include <gtest/gtest.h>

#include "antwika/input/InputCapabilities.hpp"

using antwika::input::InputCapabilities;

namespace
{
    constexpr InputCapabilities kReference{
        .keyboard = true, .pointer = false};
}

TEST(InputCapabilitiesTest, Ctor_ClaimsNoDevice)
{
    constexpr InputCapabilities none;

    EXPECT_FALSE(none.keyboard);
    EXPECT_FALSE(none.pointer);
}

TEST(InputCapabilitiesTest, OperatorEquals_IsTrueForTheSameDevices)
{
    constexpr InputCapabilities same{.keyboard = true, .pointer = false};

    EXPECT_EQ(kReference, same);
}

TEST(InputCapabilitiesTest, OperatorEquals_IsFalseWhenTheKeyboardFlagDiffers)
{
    constexpr InputCapabilities other{.keyboard = false, .pointer = false};

    EXPECT_NE(kReference, other);
}

TEST(InputCapabilitiesTest, OperatorEquals_IsFalseWhenThePointerFlagDiffers)
{
    constexpr InputCapabilities other{.keyboard = true, .pointer = true};

    EXPECT_NE(kReference, other);
}
