#include "antwika/tween/Easing.hpp"

#include <cstddef>
#include <cstdint>
#include <set>
#include <string_view>

#include <gtest/gtest.h>

using antwika::tween::Easing;
using antwika::tween::easingIndex;
using antwika::tween::easingName;
using antwika::tween::kEasingCount;

TEST(EasingTest, CountMatchesTheLastEnumerator)
{
    EXPECT_EQ(kEasingCount, 16U);
    EXPECT_EQ(easingIndex(Easing::Linear), 0U);
    EXPECT_EQ(easingIndex(Easing::BounceInOut), kEasingCount - 1);
}

TEST(EasingTest, EveryEasingHasItsOwnName)
{
    std::set<std::string_view> seen;

    for (std::size_t index = 0; index < kEasingCount; ++index)
    {
        const auto easing = static_cast<Easing>(index);
        const auto name = easingName(easing);

        EXPECT_NE(name, "unknown") << "at index " << index;
        EXPECT_TRUE(seen.insert(name).second) << "repeated " << name;
    }

    EXPECT_EQ(seen.size(), kEasingCount);
}

TEST(EasingTest, NamesTheOnesACallerWritesDown)
{
    EXPECT_EQ(easingName(Easing::Linear), "linear");
    EXPECT_EQ(easingName(Easing::CubicInOut), "cubicInOut");
    EXPECT_EQ(easingName(Easing::BounceOut), "bounceOut");
}

// Easing is a std::uint8_t, so this is a value a caller can produce.
// The name is for a message somebody is already reading.
// So it answers rather than throwing.
// That is the rule antwika::i18n's lookup follows.
TEST(EasingTest, NamesAValueNoEnumeratorHasAsUnknown)
{
    EXPECT_EQ(
        easingName(static_cast<Easing>(std::uint8_t{200})), "unknown");
}
