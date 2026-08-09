#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <set>
#include <string_view>

#include "antwika/tween/Easing.hpp"

using antwika::tween::Easing;
using antwika::tween::easingIndex;
using antwika::tween::easingName;
using antwika::tween::kEasingCount;

TEST(EasingTest, EasingIndex_MatchesTheLastEnumerator)
{
    EXPECT_EQ(kEasingCount, 16U);
    EXPECT_EQ(easingIndex(Easing::Linear), 0U);
    EXPECT_EQ(easingIndex(Easing::BounceInOut), kEasingCount - 1);
}

TEST(EasingTest, EasingName_IsUniquePerEasing)
{
    std::set<std::string_view> seen;

    for (std::size_t index = 0; index < kEasingCount; ++index)
    {
        const auto easing = static_cast<Easing>(index);
        const auto name = easingName(easing);

        EXPECT_NE(name, "unknown") << index;
        EXPECT_TRUE(seen.insert(name).second) << name;
    }

    EXPECT_EQ(seen.size(), kEasingCount);
}

TEST(EasingTest, EasingName_NamesTheCommonCurves)
{
    EXPECT_EQ(easingName(Easing::Linear), "linear");
    EXPECT_EQ(easingName(Easing::CubicInOut), "cubicInOut");
    EXPECT_EQ(easingName(Easing::BounceOut), "bounceOut");
}

TEST(EasingTest, EasingName_CallsAnUnknownValueUnknown)
{
    EXPECT_EQ(
        easingName(static_cast<Easing>(std::uint8_t{200})), "unknown");
}
