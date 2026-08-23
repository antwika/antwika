#include <gtest/gtest.h>

#include <cstdint>

#include "antwika/gfx/WindowId.hpp"

using antwika::gfx::kNullWindowId;
using antwika::gfx::getRawValue;
using antwika::gfx::WindowId;

TEST(WindowIdTest, RawValue_UnwrapsTheBackingInteger)
{
    constexpr WindowId idWindow{42};

    EXPECT_EQ(getRawValue(idWindow), std::uint64_t{42});
}

TEST(WindowIdTest, RawValue_OfTheNullIdIsZero)
{
    EXPECT_EQ(getRawValue(kNullWindowId), std::uint64_t{0});
}

TEST(WindowIdTest, OperatorEquals_DistinguishesDifferentIds)
{
    constexpr WindowId firstWindow{1};
    constexpr WindowId secondWindow{2};

    EXPECT_EQ(firstWindow, WindowId{1});
    EXPECT_NE(firstWindow, secondWindow);
}
