#include <gtest/gtest.h>

#include <cstdint>

#include "antwika/gfx/WindowId.hpp"

using antwika::gfx::kNullWindowId;
using antwika::gfx::rawValue;
using antwika::gfx::WindowId;

TEST(WindowIdTest, RawValue_UnwrapsTheBackingInteger)
{
    constexpr WindowId idWindow{42};

    EXPECT_EQ(rawValue(idWindow), std::uint64_t{42});
}

TEST(WindowIdTest, RawValue_OfTheNullIdIsZero)
{
    EXPECT_EQ(rawValue(kNullWindowId), std::uint64_t{0});
}

TEST(WindowIdTest, OperatorEquals_DistinguishesDifferentIds)
{
    constexpr WindowId firstWindow{1};
    constexpr WindowId secondWindow{2};

    EXPECT_EQ(firstWindow, WindowId{1});
    EXPECT_NE(firstWindow, secondWindow);
}
