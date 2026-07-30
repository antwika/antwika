#include <gtest/gtest.h>

#include <cstdint>

#include "antwika/gfx/WindowId.hpp"

using antwika::gfx::kNullWindowId;
using antwika::gfx::rawValue;
using antwika::gfx::WindowId;

TEST(WindowIdTest, RawValue_UnwrapsTheBackingInteger)
{
    constexpr WindowId id{42};

    EXPECT_EQ(rawValue(id), std::uint64_t{42});
}

TEST(WindowIdTest, RawValue_OfTheNullIdIsZero)
{
    EXPECT_EQ(rawValue(kNullWindowId), std::uint64_t{0});
}

TEST(WindowIdTest, Equality_DistinguishesDifferentIds)
{
    constexpr WindowId first{1};
    constexpr WindowId second{2};

    EXPECT_EQ(first, WindowId{1});
    EXPECT_NE(first, second);
}
