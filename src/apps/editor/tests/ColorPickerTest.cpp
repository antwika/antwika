#include <gtest/gtest.h>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/editor/ui/ColorPicker.hpp"

namespace
{

    using antwika::editor::bandHsv;
    using antwika::editor::bandPlace;
    using antwika::editor::colorOf;
    using antwika::editor::fieldCursorPos;
    using antwika::editor::fieldPlace;
    using antwika::editor::Hsv;
    using antwika::editor::hsvOf;
    using antwika::editor::hueBand;
    using antwika::editor::hueBandPlace;
    using antwika::editor::hueCursorPos;
    using antwika::editor::huePlace;
    using antwika::editor::kPickerBands;
    using antwika::editor::onPicker;
    using antwika::editor::colorAtPoint;
    using antwika::editor::pickerPlace;
    using antwika::gfx::PointF;
    using antwika::gfx::Color;
    using antwika::gfx::RectF;

    constexpr antwika::gfx::Size kCanvasSize{.width = 480, .height = 270};

    [[nodiscard]] bool within(const RectF outerRect, const RectF innerRect)
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
        EXPECT_TRUE(within(pickerPlace(kCanvasSize), fieldPlace(kCanvasSize)));
        EXPECT_TRUE(within(pickerPlace(kCanvasSize), huePlace(kCanvasSize)));
        EXPECT_GE(
            huePlace(kCanvasSize).originPoint.x,
            fieldPlace(kCanvasSize).originPoint.x
                + fieldPlace(kCanvasSize).size.width);
    }

    TEST(ColorPickerTest, ColorAtPoint_TakesSaturationAcrossTheField)
    {
        const auto field = fieldPlace(kCanvasSize);
        const Hsv hsv{.hue = 0.25F, .saturation = 0.0F, .value = 0.5F};
        const auto pickedColor = colorAtPoint(
            kCanvasSize,
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
        const auto field = fieldPlace(kCanvasSize);
        const auto pickedColor = colorAtPoint(
            kCanvasSize,
            Hsv{.hue = 0.25F, .saturation = 0.5F, .value = 0.5F},
            field.originPoint);

        ASSERT_TRUE(pickedColor.has_value());
        EXPECT_NEAR(pickedColor->value, 1.0F, 1e-4F);
        EXPECT_NEAR(pickedColor->saturation, 0.0F, 1e-4F);
    }

    TEST(ColorPickerTest, ColorAtPoint_TakesTheHueDownTheStrip)
    {
        const auto strip = huePlace(kCanvasSize);
        const Hsv hsv{.hue = 0.0F, .saturation = 0.6F, .value = 0.7F};
        const auto pickedColor = colorAtPoint(
            kCanvasSize,
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
        const auto panel = pickerPlace(kCanvasSize);

        EXPECT_FALSE(
            colorAtPoint(
            kCanvasSize,
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
            colorAtPoint(kCanvasSize, hsv, fieldCursorPos(kCanvasSize, hsv));

        ASSERT_TRUE(pickedColor.has_value());
        EXPECT_NEAR(pickedColor->saturation, hsv.saturation, 1e-3F);
        EXPECT_NEAR(pickedColor->value, hsv.value, 1e-3F);
    }

    TEST(ColorPickerTest, HueCursorPos_StandsWhereThePickWouldLeaveIt)
    {
        const Hsv hsv{.hue = 0.3F, .saturation = 1.0F, .value = 1.0F};
        const auto strip = huePlace(kCanvasSize);
        const auto pickedColor = colorAtPoint(
            kCanvasSize,
            hsv,
            PointF{
                strip.originPoint.x + (strip.size.width / 2.0F),
                hueCursorPos(kCanvasSize, hsv)});

        ASSERT_TRUE(pickedColor.has_value());
        EXPECT_NEAR(pickedColor->hue, hsv.hue, 1e-3F);
    }

    TEST(ColorPickerTest, OnPicker_TellsThePanelFromTheCanvasBesideIt)
    {
        EXPECT_TRUE(onPicker(kCanvasSize, fieldCursorPos(kCanvasSize, Hsv{})));
        EXPECT_FALSE(onPicker(kCanvasSize, PointF{0.0F, 0.0F}));
    }

    TEST(ColorPickerTest, BandPlace_KeepsEveryBandInTheField)
    {
        for (std::size_t row = 0; row < kPickerBands; ++row)
        {
            for (std::size_t column = 0; column < kPickerBands;
                 ++column)
            {
                EXPECT_TRUE(
                    within(
                        fieldPlace(kCanvasSize),
                        bandPlace(kCanvasSize, column, row)));
            }
        }
    }

    TEST(ColorPickerTest, BandHsv_RunsTheCornersOfTheField)
    {
        const Hsv hsv{.hue = 0.4F, .saturation = 0.0F, .value = 0.0F};
        const auto corner = bandHsv(hsv, 0, 0);
        const auto lastBandHsv =
            bandHsv(hsv, kPickerBands - 1, kPickerBands - 1);

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
                within(huePlace(kCanvasSize),
                hueBandPlace(kCanvasSize, bandIndex)));
            EXPECT_GE(hueBand(bandIndex), 0.0F);
            EXPECT_LT(hueBand(bandIndex), 1.0F);
        }
    }

}
