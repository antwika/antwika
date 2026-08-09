#include <gtest/gtest.h>

#include "antwika/input/KeyModifiers.hpp"

using antwika::input::KeyModifiers;

namespace
{
    constexpr KeyModifiers kReference{
        .shift = true, .control = false, .alt = true, .super = false};
}

TEST(KeyModifiersTest, Ctor_HoldsNothing)
{
    constexpr KeyModifiers none;

    EXPECT_FALSE(none.shift);
    EXPECT_FALSE(none.control);
    EXPECT_FALSE(none.alt);
    EXPECT_FALSE(none.super);
}

TEST(KeyModifiersTest, OperatorEquals_IsTrueForTheSameModifiers)
{
    constexpr KeyModifiers same{
        .shift = true, .control = false, .alt = true, .super = false};

    EXPECT_EQ(kReference, same);
}

TEST(KeyModifiersTest, OperatorEquals_IsFalseWhenShiftDiffers)
{
    constexpr KeyModifiers other{
        .shift = false, .control = false, .alt = true, .super = false};

    EXPECT_NE(kReference, other);
}

TEST(KeyModifiersTest, OperatorEquals_IsFalseWhenControlDiffers)
{
    constexpr KeyModifiers other{
        .shift = true, .control = true, .alt = true, .super = false};

    EXPECT_NE(kReference, other);
}

TEST(KeyModifiersTest, OperatorEquals_IsFalseWhenAltDiffers)
{
    constexpr KeyModifiers other{
        .shift = true, .control = false, .alt = false, .super = false};

    EXPECT_NE(kReference, other);
}

TEST(KeyModifiersTest, OperatorEquals_IsFalseWhenSuperDiffers)
{
    constexpr KeyModifiers other{
        .shift = true, .control = false, .alt = true, .super = true};

    EXPECT_NE(kReference, other);
}
