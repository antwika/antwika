#include <gtest/gtest.h>

#include "antwika/input/InputCapabilities.hpp"

using antwika::input::InputCapabilities;

namespace
{
    constexpr InputCapabilities kReferenceCapabilities{
        .keyboard = true, .pointer = false};
}

TEST(InputCapabilitiesTest, Ctor_ClaimsNoDevice)
{
    constexpr InputCapabilities noneCapabilities;

    EXPECT_FALSE(noneCapabilities.keyboard);
    EXPECT_FALSE(noneCapabilities.pointer);
}

TEST(InputCapabilitiesTest, OperatorEquals_IsTrueForTheSameDevices)
{
    constexpr InputCapabilities sameCapabilities{
        .keyboard = true,
        .pointer = false};

    EXPECT_EQ(kReferenceCapabilities, sameCapabilities);
}

TEST(InputCapabilitiesTest, OperatorEquals_IsFalseWhenTheKeyboardFlagDiffers)
{
    constexpr InputCapabilities otherCapabilities{
        .keyboard = false,
        .pointer = false};

    EXPECT_NE(kReferenceCapabilities, otherCapabilities);
}

TEST(InputCapabilitiesTest, OperatorEquals_IsFalseWhenThePointerFlagDiffers)
{
    constexpr InputCapabilities otherCapabilities{
        .keyboard = true,
        .pointer = true};

    EXPECT_NE(kReferenceCapabilities, otherCapabilities);
}
