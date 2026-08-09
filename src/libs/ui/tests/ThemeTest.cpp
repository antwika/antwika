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

    EXPECT_EQ(rgb(28, 30, 38), theme.panel);
    EXPECT_EQ(rgb(232, 236, 232), theme.text);
    EXPECT_EQ(rgb(120, 140, 128), theme.muted);
    EXPECT_EQ(rgb(48, 52, 64), theme.buttonIdle);
    EXPECT_EQ(rgb(68, 74, 92), theme.buttonHovered);
    EXPECT_EQ(rgb(32, 36, 44), theme.buttonPressed);
    EXPECT_EQ(rgb(240, 240, 240), theme.buttonText);
}

TEST(ThemeTest, Theme_DefaultsToADarkerFieldThanItsPanel)
{
    constexpr Theme theme{};

    EXPECT_EQ(rgb(20, 22, 28), theme.field);
    EXPECT_EQ(rgb(14, 16, 20), theme.fieldFocused);
    EXPECT_EQ(rgb(232, 236, 232), theme.caret);
    EXPECT_EQ(rgb(44, 72, 116), theme.selection);
    EXPECT_EQ(rgb(38, 84, 52), theme.highlight);
    EXPECT_EQ(rgb(30, 33, 42), theme.scrollTrack);
    EXPECT_EQ(rgb(78, 86, 106), theme.scrollThumb);
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

TEST(ThemeTest, ScaledTheme_CarriesColoursOverUntouched)
{
    constexpr Theme base{};

    const auto theme = scaledTheme(base, 4);

    EXPECT_EQ(rgb(232, 236, 232), theme.text);

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
    EXPECT_EQ(0U, theme.dividerThickness);
}

TEST(ThemeTest, ScaledTheme_ClampsAMetricThatWouldOverflow)
{
    constexpr auto kMax = std::numeric_limits<std::uint32_t>::max();

    const auto theme = scaledTheme(Theme{}, kMax);

    EXPECT_EQ(kMax, theme.padding);
}
