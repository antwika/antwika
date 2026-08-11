#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <string>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/tilemap/Rgb.hpp>

#include "antwika/map_editor/PaletteMath.hpp"

using antwika::gfx::Size;
using antwika::map_editor::chromeFor;
using antwika::map_editor::colorOf;
using antwika::map_editor::hexOfRgb;
using antwika::map_editor::Hsv;
using antwika::map_editor::hsvOfRgb;
using antwika::map_editor::rgbOfHex;
using antwika::map_editor::rgbOfHsv;
using antwika::map_editor::svSquare;
using antwika::tilemap::Rgb;

namespace
{
    Rgb rgbAt(
        const std::uint8_t red,
        const std::uint8_t green,
        const std::uint8_t blue)
    {
        return Rgb{.red = red, .green = green, .blue = blue};
    }

    [[nodiscard]] Rgb pixelAt(
        const antwika::gfx::Bitmap &bitmap,
        const std::uint32_t x,
        const std::uint32_t y)
    {
        const auto offset =
            (static_cast<std::size_t>(y) * bitmap.size.width + x)
            * antwika::gfx::kBytesPerPixel;

        return rgbAt(
            bitmap.pixels[offset],
            bitmap.pixels[offset + 1],
            bitmap.pixels[offset + 2]);
    }
}

TEST(PaletteMathTest, RgbOfHsv_ReachesEveryHueSector)
{
    EXPECT_EQ(rgbOfHsv(Hsv{0, 255, 255}), rgbAt(255, 0, 0));
    EXPECT_EQ(rgbOfHsv(Hsv{60, 255, 255}), rgbAt(255, 255, 0));
    EXPECT_EQ(rgbOfHsv(Hsv{120, 255, 255}), rgbAt(0, 255, 0));
    EXPECT_EQ(rgbOfHsv(Hsv{180, 255, 255}), rgbAt(0, 255, 255));
    EXPECT_EQ(rgbOfHsv(Hsv{240, 255, 255}), rgbAt(0, 0, 255));
    EXPECT_EQ(rgbOfHsv(Hsv{300, 255, 255}), rgbAt(255, 0, 255));
}

TEST(PaletteMathTest, RgbOfHsv_WrapsAHuePastAFullTurn)
{
    EXPECT_EQ(rgbOfHsv(Hsv{360, 255, 255}), rgbOfHsv(Hsv{0, 255, 255}));
    EXPECT_EQ(rgbOfHsv(Hsv{420, 255, 255}), rgbOfHsv(Hsv{60, 255, 255}));
}

TEST(PaletteMathTest, RgbOfHsv_YieldsGrayWithoutSaturation)
{
    EXPECT_EQ(rgbOfHsv(Hsv{200, 0, 128}), rgbAt(128, 128, 128));
}

TEST(PaletteMathTest, RgbOfHsv_YieldsBlackWithoutValue)
{
    EXPECT_EQ(rgbOfHsv(Hsv{200, 255, 0}), rgbAt(0, 0, 0));
}

TEST(PaletteMathTest, HsvOfRgb_ReportsHueZeroForGray)
{
    const auto hsv = hsvOfRgb(rgbAt(90, 90, 90));

    EXPECT_EQ(hsv.hue, 0U);
    EXPECT_EQ(hsv.saturation, 0);
    EXPECT_EQ(hsv.value, 90);
}

TEST(PaletteMathTest, HsvOfRgb_ReportsHueZeroForBlack)
{
    const auto hsv = hsvOfRgb(rgbAt(0, 0, 0));

    EXPECT_EQ(hsv.hue, 0U);
    EXPECT_EQ(hsv.saturation, 0);
    EXPECT_EQ(hsv.value, 0);
}

TEST(PaletteMathTest, HsvOfRgb_FindsTheHueOfEachDominantChannel)
{
    EXPECT_EQ(hsvOfRgb(rgbAt(255, 0, 0)).hue, 0U);
    EXPECT_EQ(hsvOfRgb(rgbAt(0, 255, 0)).hue, 120U);
    EXPECT_EQ(hsvOfRgb(rgbAt(0, 0, 255)).hue, 240U);
}

TEST(PaletteMathTest, HsvOfRgb_WrapsAHueBelowZeroBackIntoTheTurn)
{
    EXPECT_EQ(hsvOfRgb(rgbAt(255, 0, 128)).hue, 330U);
}

TEST(PaletteMathTest, HsvOfRgb_RoundTripsThroughRgbOfHsv)
{
    for (std::uint32_t hue = 0; hue < 360; hue += 30)
    {
        const Hsv start{.hue = hue, .saturation = 255, .value = 255};
        const auto back = hsvOfRgb(rgbOfHsv(start));
        const auto apart = std::min(
            (back.hue + 360 - hue) % 360, (hue + 360 - back.hue) % 360);

        EXPECT_LE(apart, 1U);
        EXPECT_EQ(back.saturation, 255);
        EXPECT_EQ(back.value, 255);
    }
}

TEST(PaletteMathTest, HexOfRgb_PadsEveryChannelToTwoDigits)
{
    EXPECT_EQ(hexOfRgb(rgbAt(0, 0, 0)), "#000000");
    EXPECT_EQ(hexOfRgb(rgbAt(255, 255, 255)), "#ffffff");
    EXPECT_EQ(hexOfRgb(rgbAt(1, 16, 171)), "#0110ab");
}

TEST(PaletteMathTest, RgbOfHex_TakesTextWithOrWithoutAHash)
{
    EXPECT_EQ(rgbOfHex("#0110ab"), rgbAt(1, 16, 171));
    EXPECT_EQ(rgbOfHex("0110ab"), rgbAt(1, 16, 171));
}

TEST(PaletteMathTest, RgbOfHex_TakesUppercaseDigits)
{
    EXPECT_EQ(rgbOfHex("#0110AB"), rgbAt(1, 16, 171));
}

TEST(PaletteMathTest, RgbOfHex_RefusesTextOfTheWrongLength)
{
    EXPECT_FALSE(rgbOfHex("").has_value());
    EXPECT_FALSE(rgbOfHex("#01").has_value());
    EXPECT_FALSE(rgbOfHex("0110abcd").has_value());
}

TEST(PaletteMathTest, RgbOfHex_RefusesADigitOutsideTheHexRange)
{
    EXPECT_FALSE(rgbOfHex("#01g0ab").has_value());
    EXPECT_FALSE(rgbOfHex("#0110a!").has_value());
    EXPECT_FALSE(rgbOfHex("######").has_value());
}

TEST(PaletteMathTest, RgbOfHex_RoundTripsThroughHexOfRgb)
{
    const auto color = rgbAt(12, 200, 47);

    EXPECT_EQ(rgbOfHex(hexOfRgb(color)), color);
}

TEST(PaletteMathTest, ColorOf_CarriesEveryChannelAcross)
{
    const auto color = colorOf(rgbAt(3, 5, 7));

    EXPECT_EQ(color.red, 3);
    EXPECT_EQ(color.green, 5);
    EXPECT_EQ(color.blue, 7);
}

TEST(PaletteMathTest, ChromeFor_DarkensTheCheckerOnLightPaper)
{
    const auto chrome = chromeFor(rgbAt(240, 240, 240));

    EXPECT_LT(chrome.checkerLight.red, 240);
    EXPECT_LT(chrome.checkerDark.red, 240);
    EXPECT_LT(chrome.checkerLight.red, chrome.checkerDark.red);
}

TEST(PaletteMathTest, ChromeFor_LightensTheCheckerOnDarkPaper)
{
    const auto chrome = chromeFor(rgbAt(12, 14, 16));

    EXPECT_GT(chrome.checkerLight.red, 12);
    EXPECT_GT(chrome.checkerDark.red, 12);
    EXPECT_GT(chrome.checkerLight.red, chrome.checkerDark.red);
}

TEST(PaletteMathTest, ChromeFor_KeepsTheVoidDarkerThanThePaper)
{
    EXPECT_LT(chromeFor(rgbAt(240, 240, 240)).voidColor.red, 240);
    EXPECT_LT(chromeFor(rgbAt(60, 60, 60)).voidColor.red, 60);
}

TEST(PaletteMathTest, ChromeFor_ContrastsTheGhostAgainstThePaper)
{
    const auto onDark = chromeFor(rgbAt(12, 14, 16));
    const auto onLight = chromeFor(rgbAt(240, 240, 240));

    EXPECT_GT(onDark.ghostEdge.red, onLight.ghostEdge.red);
    EXPECT_GT(onDark.freeMark.red, onLight.freeMark.red);
    EXPECT_GT(onDark.ghostFill.red, onLight.ghostFill.red);
}

TEST(PaletteMathTest, SvSquare_FillsTheRequestedSize)
{
    const auto square =
        svSquare(0, Size{.width = 16, .height = 8});

    EXPECT_EQ(square.size.width, 16U);
    EXPECT_EQ(square.size.height, 8U);
    EXPECT_TRUE(square.isComplete());
}

TEST(PaletteMathTest, SvSquare_GrowsSaturationLeftToRight)
{
    const auto square =
        svSquare(0, Size{.width = 16, .height = 8});

    EXPECT_EQ(pixelAt(square, 0, 0), rgbAt(255, 255, 255));
    EXPECT_EQ(pixelAt(square, 15, 0), rgbAt(255, 0, 0));
}

TEST(PaletteMathTest, SvSquare_GrowsValueBottomToTop)
{
    const auto square =
        svSquare(0, Size{.width = 16, .height = 8});

    EXPECT_EQ(pixelAt(square, 15, 7), rgbAt(0, 0, 0));
    EXPECT_GT(pixelAt(square, 15, 0).red, pixelAt(square, 15, 7).red);
}

TEST(PaletteMathTest, SvSquare_TakesTheHueItWasGiven)
{
    const auto square =
        svSquare(240, Size{.width = 4, .height = 4});

    EXPECT_EQ(pixelAt(square, 3, 0), rgbAt(0, 0, 255));
}

TEST(PaletteMathTest, Hsv_OperatorEquals_ComparesEveryField)
{
    const Hsv base{.hue = 10, .saturation = 20, .value = 30};

    EXPECT_EQ(base, (Hsv{.hue = 10, .saturation = 20, .value = 30}));
    EXPECT_NE(base, (Hsv{.hue = 11, .saturation = 20, .value = 30}));
    EXPECT_NE(base, (Hsv{.hue = 10, .saturation = 21, .value = 30}));
    EXPECT_NE(base, (Hsv{.hue = 10, .saturation = 20, .value = 31}));
}
