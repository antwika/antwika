#include <gtest/gtest.h>

#include "antwika/gfx/ColorSpace.hpp"

namespace
{

    using antwika::gfx::Color;
    using antwika::gfx::colorOf;
    using antwika::gfx::Hsv;
    using antwika::gfx::hsvOf;

    TEST(ColorSpaceTest, ColorOf_GivesTheHuesRoundTheWheel)
    {
        EXPECT_EQ(
            colorOf(Hsv{.hue = 0.0F, .saturation = 1.0F, .value = 1.0F}),
            (Color{.red = 255, .green = 0, .blue = 0}));
        EXPECT_EQ(
            colorOf(
                Hsv{
                    .hue = 1.0F / 3.0F,
                    .saturation = 1.0F,
                    .value = 1.0F}),
            (Color{.red = 0, .green = 255, .blue = 0}));
        EXPECT_EQ(
            colorOf(
                Hsv{
                    .hue = 2.0F / 3.0F,
                    .saturation = 1.0F,
                    .value = 1.0F}),
            (Color{.red = 0, .green = 0, .blue = 255}));
    }

    TEST(ColorSpaceTest, ColorOf_GivesGreyWithoutSaturation)
    {
        EXPECT_EQ(
            colorOf(Hsv{.hue = 0.5F, .saturation = 0.0F, .value = 0.5F}),
            (Color{.red = 128, .green = 128, .blue = 128}));
    }

    TEST(ColorSpaceTest, ColorOf_GivesBlackWithoutValue)
    {
        EXPECT_EQ(
            colorOf(Hsv{.hue = 0.5F, .saturation = 1.0F, .value = 0.0F}),
            (Color{.red = 0, .green = 0, .blue = 0}));
    }

    TEST(ColorSpaceTest, ColorOf_TakesAWholeTurnForNone)
    {
        EXPECT_EQ(
            colorOf(Hsv{.hue = 1.0F, .saturation = 1.0F, .value = 1.0F}),
            colorOf(
                Hsv{.hue = 0.0F, .saturation = 1.0F, .value = 1.0F}));
    }

    TEST(ColorSpaceTest, HsvOf_GivesBackWhatColorOfWasTold)
    {
        for (const auto color :
             {Color{.red = 214, .green = 96, .blue = 84},
              Color{.red = 92, .green = 168, .blue = 124},
              Color{.red = 20, .green = 20, .blue = 28},
              Color{.red = 232, .green = 232, .blue = 224}})
        {
            EXPECT_EQ(colorOf(hsvOf(color)), color);
        }
    }

    TEST(ColorSpaceTest, ColorToHex_WritesAHashAndSixDigits)
    {
        EXPECT_EQ(
            antwika::gfx::getColorToHex(
                Color{.red = 214, .green = 96, .blue = 84}),
            "#d66054");
    }

    TEST(ColorSpaceTest, ColorFromHex_ReadsBackWhatColorToHexWrote)
    {
        const Color color{.red = 20, .green = 168, .blue = 224};
        const auto parsedColor =
            antwika::gfx::getColorFromHex(antwika::gfx::getColorToHex(color));

        ASSERT_TRUE(parsedColor.has_value());
        EXPECT_EQ(*parsedColor, color);
    }

    TEST(ColorSpaceTest, ColorFromHex_TakesEitherCaseAndNoHash)
    {
        const auto parsedColor = antwika::gfx::getColorFromHex("14A8E0");

        ASSERT_TRUE(parsedColor.has_value());
        EXPECT_EQ(
            *parsedColor, (Color{.red = 20, .green = 168, .blue = 224}));
    }

    TEST(ColorSpaceTest, ColorFromHex_ReadsNothingFromWhatIsNoColor)
    {
        EXPECT_FALSE(antwika::gfx::getColorFromHex("").has_value());
        EXPECT_FALSE(antwika::gfx::getColorFromHex("#12345").has_value());
        EXPECT_FALSE(
            antwika::gfx::getColorFromHex("#12345g").has_value());
    }

}
