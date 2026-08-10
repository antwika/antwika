#include <gtest/gtest.h>

#include <antwika/tilemap/Rgb.hpp>

using antwika::tilemap::Rgb;

namespace
{
    template <typename Mutate>
    void expectMemberCompared(const Rgb &base, Mutate mutate)
    {
        Rgb changed = base;
        mutate(changed);

        const Rgb twin = base;

        EXPECT_NE(base, changed);
        EXPECT_EQ(base, twin);
    }
}

TEST(RgbTest, Rgb_DefaultsToBlack)
{
    const Rgb color{};

    EXPECT_EQ(color.red, 0);
    EXPECT_EQ(color.green, 0);
    EXPECT_EQ(color.blue, 0);
}

TEST(RgbTest, OperatorEquals_ComparesEveryField)
{
    const Rgb base{.red = 10, .green = 20, .blue = 30};

    expectMemberCompared(base, [](Rgb &color) { color.red = 11; });
    expectMemberCompared(base, [](Rgb &color) { color.green = 21; });
    expectMemberCompared(base, [](Rgb &color) { color.blue = 31; });
}
