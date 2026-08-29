#include <gtest/gtest.h>

#include <array>
#include <cstddef>

#include <antwika/component/Health.hpp>

#include "antwika/render/HealthBars.hpp"

using antwika::component::Health;
using antwika::component::kFullHealth;
using antwika::render::getHealthBars;
using antwika::render::kHealthBarGap;
using antwika::render::kHealthBarTall;
using antwika::render::kHealthBarWide;

namespace
{

    constexpr float kTolerance = 0.001F;

    constexpr std::array<std::size_t, 2> kGroundBars{0U, 2U};

TEST(HealthBarsTest, HealthBars_PutFoodAboveWater)
{
    const auto bars = getHealthBars({100.0F, 50.0F}, Health{});

    EXPECT_LT(bars.at(0).originPoint.y, bars.at(2).originPoint.y);
    EXPECT_NEAR(
        bars.at(2).originPoint.y - bars.at(0).originPoint.y,
        kHealthBarTall + kHealthBarGap,
        kTolerance);
    EXPECT_LT(bars.at(2).originPoint.y, 50.0F);
}
TEST(HealthBarsTest, HealthBars_CentreBothBarsOverThePointGiven)
{
    const auto bars = getHealthBars({100.0F, 50.0F}, Health{});

    for (const auto which : kGroundBars)
    {
        EXPECT_NEAR(
            bars.at(which).originPoint.x,
            100.0F - (kHealthBarWide / 2.0F),
            kTolerance);
        EXPECT_NEAR(
            bars.at(which).size.width, kHealthBarWide, kTolerance);
        EXPECT_NEAR(
            bars.at(which).size.height, kHealthBarTall, kTolerance);
    }
}
TEST(HealthBarsTest, HealthBars_FillEachAsAShareOfTheWhole)
{
    const auto bars = getHealthBars(
        {100.0F, 50.0F},
        Health{
            .food = kFullHealth,
            .water = static_cast<std::uint16_t>(kFullHealth / 2)});

    EXPECT_NEAR(bars.at(1).size.width, kHealthBarWide, kTolerance);
    EXPECT_NEAR(
        bars.at(3).size.width, kHealthBarWide / 2.0F, kTolerance);
    EXPECT_NEAR(
        bars.at(1).originPoint.x, bars.at(0).originPoint.x, kTolerance);
    EXPECT_NEAR(
        bars.at(3).originPoint.y, bars.at(2).originPoint.y, kTolerance);
}
TEST(HealthBarsTest, HealthBars_FillNothingForADepletedLevel)
{
    const auto bars =
        getHealthBars({100.0F, 50.0F}, Health{.food = 0, .water = 0});

    EXPECT_NEAR(bars.at(1).size.width, 0.0F, kTolerance);
    EXPECT_NEAR(bars.at(3).size.width, 0.0F, kTolerance);
}
TEST(HealthBarsTest, HealthBars_LandOnWholeCanvasPixels)
{
    const auto bars = getHealthBars(
        {100.3F, 50.7F},
        Health{.food = 7, .water = kFullHealth});

    for (const auto &bar : bars)
    {
        EXPECT_NEAR(
            bar.originPoint.x, std::round(bar.originPoint.x), kTolerance);
        EXPECT_NEAR(
            bar.originPoint.y, std::round(bar.originPoint.y), kTolerance);
        EXPECT_NEAR(
            bar.size.width,
            std::round(bar.size.width),
            kTolerance);
    }
}

}
