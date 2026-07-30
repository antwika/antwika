#include <gtest/gtest.h>

#include <cstdint>
#include <limits>

#include <antwika/gfx/Size.hpp>

#include "antwika/ui/Theme.hpp"

using antwika::gfx::Size;
using antwika::ui::scaledTheme;
using antwika::ui::scaleForCanvas;
using antwika::ui::Theme;

TEST(ThemeTest, Theme_DefaultsToReadableMetrics)
{
    constexpr Theme theme{};

    EXPECT_EQ(1U, theme.textScale);
    EXPECT_EQ(4U, theme.padding);
    EXPECT_EQ(4U, theme.gap);
    EXPECT_EQ(6U, theme.buttonPadding);
}

TEST(ThemeTest, ScaleForCanvas_IsOneForASmallCanvas)
{
    EXPECT_EQ(
        1U, scaleForCanvas(Size{.width = 320, .height = 100}));
}

TEST(ThemeTest, ScaleForCanvas_GrowsWithTheCanvasHeight)
{
    EXPECT_EQ(
        3U, scaleForCanvas(Size{.width = 1280, .height = 720}));
}

TEST(ThemeTest, ScaledTheme_MultipliesEveryMetric)
{
    const auto theme = scaledTheme(Theme{}, 3);

    EXPECT_EQ(3U, theme.textScale);
    EXPECT_EQ(12U, theme.padding);
    EXPECT_EQ(12U, theme.gap);
    EXPECT_EQ(18U, theme.buttonPadding);
}

// A colour does not get bigger on a bigger screen.
TEST(ThemeTest, ScaledTheme_CarriesColoursOverUntouched)
{
    constexpr Theme base{};

    const auto theme = scaledTheme(base, 4);

    EXPECT_EQ(base.panel, theme.panel);
    EXPECT_EQ(base.text, theme.text);
    EXPECT_EQ(base.muted, theme.muted);
    EXPECT_EQ(base.buttonIdle, theme.buttonIdle);
    EXPECT_EQ(base.buttonHovered, theme.buttonHovered);
    EXPECT_EQ(base.buttonPressed, theme.buttonPressed);
    EXPECT_EQ(base.buttonText, theme.buttonText);
}

TEST(ThemeTest, ScaledTheme_AtZeroLeavesNoMetrics)
{
    const auto theme = scaledTheme(Theme{}, 0);

    EXPECT_EQ(0U, theme.textScale);
    EXPECT_EQ(0U, theme.padding);
    EXPECT_EQ(0U, theme.gap);
    EXPECT_EQ(0U, theme.buttonPadding);
}

TEST(ThemeTest, ScaledTheme_ClampsAMetricThatWouldOverflow)
{
    constexpr auto kMax = std::numeric_limits<std::uint32_t>::max();

    const auto theme = scaledTheme(Theme{}, kMax);

    EXPECT_EQ(kMax, theme.padding);
}
