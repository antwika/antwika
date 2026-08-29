#include <gtest/gtest.h>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/editor/ui/ColorPicker.hpp"

namespace
{

    using antwika::editor::getBandHsv;
    using antwika::editor::getBandPlace;
    using antwika::editor::colorOf;
    using antwika::editor::getFieldCursorPos;
    using antwika::editor::getFieldPlace;
    using antwika::editor::Hsv;
    using antwika::editor::hsvOf;
    using antwika::editor::getHueBand;
    using antwika::editor::getHueBandPlace;
    using antwika::editor::getHueCursorPos;
    using antwika::editor::getHuePlace;
    using antwika::editor::kPickerBands;
    using antwika::editor::isOnPicker;
    using antwika::editor::getColorAtPoint;
    using antwika::editor::getPickerPlace;
    using antwika::gfx::PointF;
    using antwika::gfx::Color;
    using antwika::gfx::RectF;

    constexpr antwika::gfx::Size kCanvasSize{.width = 480, .height = 270};

    constexpr float kRailWidth = antwika::editor::kRightPanelWidth;

    [[nodiscard]] bool isWithin(const RectF outerRect, const RectF innerRect)
    {
        return innerRect.originPoint.x >= outerRect.originPoint.x
               && innerRect.originPoint.y >= outerRect.originPoint.y
               && innerRect.originPoint.x + innerRect.size.width
                      <= outerRect.originPoint.x + outerRect.size.width
               && innerRect.originPoint.y + innerRect.size.height
                      <= outerRect.originPoint.y + outerRect.size.height;
    }

    TEST(ColorPickerTest, PickerPlace_HoldsTheFieldAndTheStrip)
    {
        EXPECT_TRUE(
            isWithin(
                getPickerPlace(kCanvasSize, kRailWidth),
                getFieldPlace(kCanvasSize, kRailWidth)));
        EXPECT_TRUE(
            isWithin(
                getPickerPlace(kCanvasSize, kRailWidth),
                getHuePlace(kCanvasSize, kRailWidth)));
        EXPECT_GE(
            getHuePlace(kCanvasSize, kRailWidth).originPoint.x,
            getFieldPlace(kCanvasSize, kRailWidth).originPoint.x
                + getFieldPlace(kCanvasSize, kRailWidth).size.width);
    }

    TEST(ColorPickerTest, ColorAtPoint_TakesSaturationAcrossTheField)
    {
        const auto field = getFieldPlace(kCanvasSize, kRailWidth);
        const Hsv hsv{.hue = 0.25F, .saturation = 0.0F, .value = 0.5F};
        const auto pickedColor = getColorAtPoint(
            kCanvasSize, kRailWidth,
            hsv,
            PointF{
                field.originPoint.x + field.size.width,
                field.originPoint.y + field.size.height});

        ASSERT_TRUE(pickedColor.has_value());
        EXPECT_NEAR(pickedColor->hue, 0.25F, 1e-4F);
        EXPECT_NEAR(pickedColor->saturation, 1.0F, 1e-4F);
        EXPECT_NEAR(pickedColor->value, 0.0F, 1e-4F);
    }

    TEST(ColorPickerTest, ColorAtPoint_TakesValueUpTheField)
    {
        const auto field = getFieldPlace(kCanvasSize, kRailWidth);
        const auto pickedColor = getColorAtPoint(
            kCanvasSize, kRailWidth,
            Hsv{.hue = 0.25F, .saturation = 0.5F, .value = 0.5F},
            field.originPoint);

        ASSERT_TRUE(pickedColor.has_value());
        EXPECT_NEAR(pickedColor->value, 1.0F, 1e-4F);
        EXPECT_NEAR(pickedColor->saturation, 0.0F, 1e-4F);
    }

    TEST(ColorPickerTest, ColorAtPoint_TakesTheHueDownTheStrip)
    {
        const auto strip = getHuePlace(kCanvasSize, kRailWidth);
        const Hsv hsv{.hue = 0.0F, .saturation = 0.6F, .value = 0.7F};
        const auto pickedColor = getColorAtPoint(
            kCanvasSize, kRailWidth,
            hsv,
            PointF{
                strip.originPoint.x + (strip.size.width / 2.0F),
                strip.originPoint.y + (strip.size.height / 2.0F)});

        ASSERT_TRUE(pickedColor.has_value());
        EXPECT_NEAR(pickedColor->hue, 0.5F, 1e-2F);
        EXPECT_NEAR(pickedColor->saturation, hsv.saturation, 1e-4F);
        EXPECT_NEAR(pickedColor->value, hsv.value, 1e-4F);
    }

    TEST(ColorPickerTest, ColorAtPoint_FindsNothingBesideThePicker)
    {
        const auto panel = getPickerPlace(kCanvasSize, kRailWidth);

        EXPECT_FALSE(
            getColorAtPoint(
            kCanvasSize, kRailWidth,
                Hsv{},
                PointF{
                    panel.originPoint.x + panel.size.width + 8.0F,
                    panel.originPoint.y})
                .has_value());
    }

    TEST(ColorPickerTest, FieldCursorPos_StandsWhereThePickWouldLeaveIt)
    {
        const Hsv hsv{
            .hue = 0.75F, .saturation = 0.25F, .value = 0.8F};
        const auto pickedColor =
            getColorAtPoint(
                kCanvasSize,
                kRailWidth,
                hsv,
                getFieldCursorPos(kCanvasSize, kRailWidth, hsv));

        ASSERT_TRUE(pickedColor.has_value());
        EXPECT_NEAR(pickedColor->saturation, hsv.saturation, 1e-3F);
        EXPECT_NEAR(pickedColor->value, hsv.value, 1e-3F);
    }

    TEST(ColorPickerTest, HueCursorPos_StandsWhereThePickWouldLeaveIt)
    {
        const Hsv hsv{.hue = 0.3F, .saturation = 1.0F, .value = 1.0F};
        const auto strip = getHuePlace(kCanvasSize, kRailWidth);
        const auto pickedColor = getColorAtPoint(
            kCanvasSize, kRailWidth,
            hsv,
            PointF{
                strip.originPoint.x + (strip.size.width / 2.0F),
                getHueCursorPos(kCanvasSize, kRailWidth, hsv)});

        ASSERT_TRUE(pickedColor.has_value());
        EXPECT_NEAR(pickedColor->hue, hsv.hue, 1e-3F);
    }

    TEST(ColorPickerTest, OnPicker_TellsThePanelFromTheCanvasBesideIt)
    {
        EXPECT_TRUE(
            isOnPicker(
                kCanvasSize,
                kRailWidth,
                getFieldCursorPos(kCanvasSize, kRailWidth, Hsv{})));
        EXPECT_FALSE(isOnPicker(kCanvasSize, kRailWidth, PointF{0.0F, 0.0F}));
    }

    TEST(ColorPickerTest, BandPlace_KeepsEveryBandInTheField)
    {
        for (std::size_t row = 0; row < kPickerBands; ++row)
        {
            for (std::size_t column = 0; column < kPickerBands;
                 ++column)
            {
                EXPECT_TRUE(
                    isWithin(
                        getFieldPlace(kCanvasSize, kRailWidth),
                        getBandPlace(kCanvasSize, kRailWidth, column, row)));
            }
        }
    }

    TEST(ColorPickerTest, BandHsv_RunsTheCornersOfTheField)
    {
        const Hsv hsv{.hue = 0.4F, .saturation = 0.0F, .value = 0.0F};
        const auto corner = getBandHsv(hsv, 0, 0);
        const auto lastBandHsv =
            getBandHsv(hsv, kPickerBands - 1, kPickerBands - 1);

        EXPECT_NEAR(corner.saturation, 0.0F, 1e-4F);
        EXPECT_NEAR(corner.value, 1.0F, 1e-4F);
        EXPECT_NEAR(lastBandHsv.saturation, 1.0F, 1e-4F);
        EXPECT_NEAR(lastBandHsv.value, 0.0F, 1e-4F);
        EXPECT_NEAR(corner.hue, hsv.hue, 1e-4F);
    }

    TEST(ColorPickerTest, HueBandPlace_KeepsEveryBandInTheStrip)
    {
        for (std::size_t bandIndex = 0; bandIndex < kPickerBands; ++bandIndex)
        {
            EXPECT_TRUE(
                isWithin(getHuePlace(kCanvasSize, kRailWidth),
                getHueBandPlace(kCanvasSize, kRailWidth, bandIndex)));
            EXPECT_GE(getHueBand(bandIndex), 0.0F);
            EXPECT_LT(getHueBand(bandIndex), 1.0F);
        }
    }

}

TEST(ColorPickerTest, PickerPlace_StandsClearOfAWiderRail)
{
    const auto restingPlace = getPickerPlace(kCanvasSize, kRailWidth);
    const auto widePlace = getPickerPlace(kCanvasSize, kRailWidth * 2.0F);

    EXPECT_FLOAT_EQ(
        restingPlace.originPoint.x - widePlace.originPoint.x, kRailWidth);
    EXPECT_FLOAT_EQ(widePlace.size.width, restingPlace.size.width);
}

TEST(ColorPickerTest, ColorAtPoint_FollowsThePickerUnderAWiderRail)
{
    const auto widePlace = getPickerPlace(kCanvasSize, kRailWidth * 2.0F);
    const PointF middlePoint{
        widePlace.originPoint.x + (widePlace.size.width / 2.0F),
        widePlace.originPoint.y + (widePlace.size.height / 2.0F)};

    EXPECT_TRUE(
        getColorAtPoint(kCanvasSize, kRailWidth * 2.0F, Hsv{}, middlePoint)
            .has_value());
    EXPECT_FALSE(
        getColorAtPoint(kCanvasSize, kRailWidth, Hsv{}, middlePoint)
            .has_value());
}
