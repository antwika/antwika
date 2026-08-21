#include <gtest/gtest.h>

#include "antwika/input/KeyModifiers.hpp"

using antwika::input::KeyModifiers;

namespace
{
    constexpr KeyModifiers kReferenceModifiers{
        .shift = true, .control = false, .alt = true, .super = false};
}

TEST(KeyModifiersTest, Ctor_HoldsNothing)
{
    constexpr KeyModifiers noneModifiers;

    EXPECT_FALSE(noneModifiers.shift);
    EXPECT_FALSE(noneModifiers.control);
    EXPECT_FALSE(noneModifiers.alt);
    EXPECT_FALSE(noneModifiers.super);
}

TEST(KeyModifiersTest, OperatorEquals_IsTrueForTheSameModifiers)
{
    constexpr KeyModifiers sameModifiers{
        .shift = true, .control = false, .alt = true, .super = false};

    EXPECT_EQ(kReferenceModifiers, sameModifiers);
}

TEST(KeyModifiersTest, OperatorEquals_IsFalseWhenShiftDiffers)
{
    constexpr KeyModifiers otherModifiers{
        .shift = false, .control = false, .alt = true, .super = false};

    EXPECT_NE(kReferenceModifiers, otherModifiers);
}

TEST(KeyModifiersTest, OperatorEquals_IsFalseWhenControlDiffers)
{
    constexpr KeyModifiers otherModifiers{
        .shift = true, .control = true, .alt = true, .super = false};

    EXPECT_NE(kReferenceModifiers, otherModifiers);
}

TEST(KeyModifiersTest, OperatorEquals_IsFalseWhenAltDiffers)
{
    constexpr KeyModifiers otherModifiers{
        .shift = true, .control = false, .alt = false, .super = false};

    EXPECT_NE(kReferenceModifiers, otherModifiers);
}

TEST(KeyModifiersTest, OperatorEquals_IsFalseWhenSuperDiffers)
{
    constexpr KeyModifiers otherModifiers{
        .shift = true, .control = false, .alt = true, .super = true};

    EXPECT_NE(kReferenceModifiers, otherModifiers);
}
