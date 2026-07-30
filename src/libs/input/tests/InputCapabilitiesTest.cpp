#include <gtest/gtest.h>

#include "antwika/input/InputCapabilities.hpp"

using antwika::input::InputCapabilities;

namespace
{
    constexpr InputCapabilities kReference{
        .keyboard = true, .pointer = false};
} // namespace

TEST(InputCapabilitiesTest, DefaultConstruction_ClaimsNoDevice)
{
    // A backend has to say what it deals in, not inherit a claim.
    constexpr InputCapabilities none;

    EXPECT_FALSE(none.keyboard);
    EXPECT_FALSE(none.pointer);
}

TEST(InputCapabilitiesTest, Equality_IsTrueForTheSameDevices)
{
    constexpr InputCapabilities same{.keyboard = true, .pointer = false};

    EXPECT_EQ(kReference, same);
}

TEST(InputCapabilitiesTest, Equality_IsFalseWhenTheKeyboardFlagDiffers)
{
    constexpr InputCapabilities other{.keyboard = false, .pointer = false};

    EXPECT_NE(kReference, other);
}

TEST(InputCapabilitiesTest, Equality_IsFalseWhenThePointerFlagDiffers)
{
    constexpr InputCapabilities other{.keyboard = true, .pointer = true};

    EXPECT_NE(kReference, other);
}
