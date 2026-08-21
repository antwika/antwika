#include <gtest/gtest.h>

#include <cstdint>
#include <limits>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/ui/Theme.hpp"

using antwika::gfx::Color;
using antwika::gfx::Size;
using antwika::ui::scaledTheme;
using antwika::ui::scaleForCanvas;
using antwika::ui::Theme;

namespace
{
    [[nodiscard]] constexpr Color rgb(
        std::uint8_t red, std::uint8_t green, std::uint8_t blue)
    {
        return Color{.red = red, .green = green, .blue = blue};
    }
}

TEST(ThemeTest, Theme_DefaultsToReadableMetrics)
{
    constexpr Theme theme{};

    EXPECT_EQ(1U, theme.textScale);
    EXPECT_EQ(4U, theme.padding);
    EXPECT_EQ(4U, theme.gap);
    EXPECT_EQ(6U, theme.buttonPadding);
    EXPECT_EQ(8U, theme.scrollbarWidth);
}

TEST(ThemeTest, Theme_DefaultsToADarkChromeWithLightInk)
{
    constexpr Theme theme{};

    EXPECT_EQ(rgb(28, 30, 38), theme.panelColor);
    EXPECT_EQ(rgb(232, 236, 232), theme.textColor);
    EXPECT_EQ(rgb(120, 140, 128), theme.mutedColor);
    EXPECT_EQ(rgb(48, 52, 64), theme.buttonIdleColor);
    EXPECT_EQ(rgb(68, 74, 92), theme.buttonHoveredColor);
    EXPECT_EQ(rgb(32, 36, 44), theme.buttonPressedColor);
    EXPECT_EQ(rgb(240, 240, 240), theme.buttonTextColor);
}

TEST(ThemeTest, Theme_DefaultsToADarkerFieldThanItsPanel)
{
    constexpr Theme theme{};

    EXPECT_EQ(rgb(20, 22, 28), theme.fieldColor);
    EXPECT_EQ(rgb(14, 16, 20), theme.fieldFocusedColor);
    EXPECT_EQ(rgb(232, 236, 232), theme.caretColor);
    EXPECT_EQ(rgb(44, 72, 116), theme.selectionColor);
    EXPECT_EQ(rgb(38, 84, 52), theme.highlightColor);
    EXPECT_EQ(rgb(30, 33, 42), theme.scrollTrackColor);
    EXPECT_EQ(rgb(78, 86, 106), theme.scrollThumbColor);
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
    EXPECT_EQ(18U, theme.dividerThickness);
}

TEST(ThemeTest, ScaledTheme_CarriesColorsOverUntouched)
{
    constexpr Theme baseTheme{};

    const auto theme = scaledTheme(baseTheme, 4);

    EXPECT_EQ(rgb(232, 236, 232), theme.textColor);

    EXPECT_EQ(baseTheme.panelColor, theme.panelColor);
    EXPECT_EQ(baseTheme.textColor, theme.textColor);
    EXPECT_EQ(baseTheme.mutedColor, theme.mutedColor);
    EXPECT_EQ(baseTheme.buttonIdleColor, theme.buttonIdleColor);
    EXPECT_EQ(baseTheme.buttonHoveredColor, theme.buttonHoveredColor);
    EXPECT_EQ(baseTheme.buttonPressedColor, theme.buttonPressedColor);
    EXPECT_EQ(baseTheme.buttonTextColor, theme.buttonTextColor);
}

TEST(ThemeTest, ScaledTheme_AtZeroLeavesNoMetrics)
{
    const auto theme = scaledTheme(Theme{}, 0);

    EXPECT_EQ(0U, theme.textScale);
    EXPECT_EQ(0U, theme.padding);
    EXPECT_EQ(0U, theme.gap);
    EXPECT_EQ(0U, theme.buttonPadding);
    EXPECT_EQ(0U, theme.dividerThickness);
}

TEST(ThemeTest, ScaledTheme_ClampsAMetricThatWouldOverflow)
{
    constexpr auto kMax = std::numeric_limits<std::uint32_t>::max();

    const auto theme = scaledTheme(Theme{}, kMax);

    EXPECT_EQ(kMax, theme.padding);
}
