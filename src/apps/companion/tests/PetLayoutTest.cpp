#include <array>
#include <cstdint>

#include <gtest/gtest.h>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/companion/PetLayout.hpp"

using antwika::companion::box;
using antwika::companion::kSceneUnits;
using antwika::companion::layoutFor;
using antwika::companion::point;
using antwika::companion::Prop;
using antwika::companion::propAt;
using antwika::companion::propBox;
using antwika::companion::reviveButtonBox;
using antwika::companion::reviveButtonRect;
using antwika::companion::withinReviveButton;
using antwika::gfx::Point;
using antwika::gfx::Rect;
using antwika::gfx::Size;

namespace
{
    // 256 pixels square is what main.cpp asks for.
    // 32 whole units a side divides into it exactly eight pixels each.
    constexpr Size kCanvas{.width = 256, .height = 256};

    constexpr std::array<Prop, 3> kProps{
        Prop::Bowl, Prop::Ball, Prop::Nest};

    [[nodiscard]] Point middleOf(const Rect &area)
    {
        return Point{
            .x = area.origin.x
                 + static_cast<std::int32_t>(area.size.width) / 2,
            .y = area.origin.y
                 + static_cast<std::int32_t>(area.size.height) / 2};
    }

    [[nodiscard]] bool overlap(const Rect &one, const Rect &other)
    {
        const auto right = [](const Rect &area)
        {
            return area.origin.x
                   + static_cast<std::int32_t>(area.size.width);
        };
        const auto bottom = [](const Rect &area)
        {
            return area.origin.y
                   + static_cast<std::int32_t>(area.size.height);
        };

        return one.origin.x < right(other) && other.origin.x < right(one)
               && one.origin.y < bottom(other)
               && other.origin.y < bottom(one);
    }

    TEST(PetLayoutTest, LayoutFor_GivesEveryUnitAWholeNumberOfPixels)
    {
        const auto layout = layoutFor(kCanvas);

        ASSERT_TRUE(layout.has_value());
        EXPECT_EQ(layout->unit, kCanvas.width / kSceneUnits);
        EXPECT_EQ(layout->origin, (Point{.x = 0, .y = 0}));
    }

    TEST(PetLayoutTest, LayoutFor_CentresTheSquareGridOnTheLongerSide)
    {
        const auto layout =
            layoutFor(Size{.width = 320, .height = 256});

        ASSERT_TRUE(layout.has_value());
        EXPECT_EQ(layout->unit, 8);
        EXPECT_EQ(layout->origin, (Point{.x = 32, .y = 0}));
    }

    TEST(PetLayoutTest, LayoutFor_CentresItOnATallCanvasToo)
    {
        const auto layout =
            layoutFor(Size{.width = 256, .height = 320});

        ASSERT_TRUE(layout.has_value());
        EXPECT_EQ(layout->unit, 8);
        EXPECT_EQ(layout->origin, (Point{.x = 0, .y = 32}));
    }

    TEST(PetLayoutTest, LayoutFor_AnswersNothingForACanvasWithNoRoom)
    {
        EXPECT_FALSE(
            layoutFor(Size{.width = 16, .height = 16}).has_value());
    }

    TEST(PetLayoutTest, Box_IsMeasuredInWholeUnitsFromTheOrigin)
    {
        const auto layout = layoutFor(kCanvas);
        ASSERT_TRUE(layout.has_value());

        EXPECT_EQ(point(*layout, 2, 3), (Point{.x = 16, .y = 24}));
        EXPECT_EQ(
            box(*layout, 2, 3, 4, 5),
            (Rect{
                .origin = {.x = 16, .y = 24},
                .size = {.width = 32, .height = 40}}));
    }

    // The one rectangle the scene paints and the sink hit-tests.
    // Both overloads are the same box, which is the whole point.
    TEST(PetLayoutTest, ReviveButton_IsOneRectangleWhicheverWayItIsAsked)
    {
        const auto layout = layoutFor(kCanvas);
        ASSERT_TRUE(layout.has_value());

        const auto byCanvas = reviveButtonRect(kCanvas);

        ASSERT_TRUE(byCanvas.has_value());
        EXPECT_EQ(*byCanvas, reviveButtonBox(*layout));
    }

    TEST(PetLayoutTest, ReviveButtonRect_AnswersNothingWithoutAGrid)
    {
        EXPECT_FALSE(
            reviveButtonRect(Size{.width = 16, .height = 16})
                .has_value());
    }

    TEST(PetLayoutTest, WithinReviveButton_AcceptsAPressInsideIt)
    {
        const auto button = reviveButtonRect(kCanvas);
        ASSERT_TRUE(button.has_value());

        EXPECT_TRUE(withinReviveButton(kCanvas, button->origin));
        EXPECT_TRUE(withinReviveButton(
            kCanvas,
            Point{
                .x = button->origin.x
                     + static_cast<std::int32_t>(button->size.width) - 1,
                .y = button->origin.y
                     + static_cast<std::int32_t>(button->size.height)
                       - 1}));
    }

    // Half-open in both axes.
    // So two boxes sharing an edge cannot both claim the pixel on it.
    TEST(PetLayoutTest, WithinReviveButton_RefusesAPressOnItsFarEdge)
    {
        const auto button = reviveButtonRect(kCanvas);
        ASSERT_TRUE(button.has_value());

        EXPECT_FALSE(withinReviveButton(
            kCanvas,
            Point{
                .x = button->origin.x
                     + static_cast<std::int32_t>(button->size.width),
                .y = button->origin.y}));
        EXPECT_FALSE(withinReviveButton(
            kCanvas,
            Point{
                .x = button->origin.x,
                .y = button->origin.y
                     + static_cast<std::int32_t>(
                         button->size.height)}));
    }

    TEST(PetLayoutTest, WithinReviveButton_RefusesAPressAboveAndLeftOfIt)
    {
        const auto button = reviveButtonRect(kCanvas);
        ASSERT_TRUE(button.has_value());

        EXPECT_FALSE(withinReviveButton(
            kCanvas,
            Point{.x = button->origin.x - 1, .y = button->origin.y}));
        EXPECT_FALSE(withinReviveButton(
            kCanvas,
            Point{.x = button->origin.x, .y = button->origin.y - 1}));
    }

    // A window with no room for the grid has no button to press.
    TEST(PetLayoutTest, WithinReviveButton_RefusesEverythingWithoutAGrid)
    {
        EXPECT_FALSE(withinReviveButton(
            Size{.width = 16, .height = 16}, Point{.x = 8, .y = 8}));
    }

    // The button covers none of the grave, the gauges or the props.
    // Which is the whole reason it sits where it does.
    TEST(PetLayoutTest, ReviveButton_CoversNothingElseThePictureDraws)
    {
        const auto layout = layoutFor(kCanvas);
        ASSERT_TRUE(layout.has_value());

        const Rect button = reviveButtonBox(*layout);
        const auto unit = static_cast<std::int32_t>(layout->unit);

        // Below all four gauges, which end at unit row eight.
        EXPECT_GE(button.origin.y, 8 * unit);

        // And clear of the grave, whose stone starts at unit row twelve.
        EXPECT_LE(
            button.origin.y
                + static_cast<std::int32_t>(button.size.height),
            12 * unit);

        for (const Prop prop : kProps)
        {
            EXPECT_FALSE(overlap(button, propBox(*layout, prop)));
        }
    }

    // Every prop is asked for by the function the scene paints from.
    // So aiming at one and hitting it are one rectangle.
    TEST(PetLayoutTest, PropAt_FindsEachPropInTheMiddleOfItsOwnBox)
    {
        const auto layout = layoutFor(kCanvas);
        ASSERT_TRUE(layout.has_value());

        for (const Prop prop : kProps)
        {
            const auto found =
                propAt(kCanvas, middleOf(propBox(*layout, prop)));

            ASSERT_TRUE(found.has_value());
            EXPECT_EQ(*found, prop);
        }
    }

    // A press has to mean exactly one thing.
    // So no two props may share so much as a single pixel.
    TEST(PetLayoutTest, PropBox_NoTwoPropsOverlap)
    {
        const auto layout = layoutFor(kCanvas);
        ASSERT_TRUE(layout.has_value());

        EXPECT_FALSE(overlap(
            propBox(*layout, Prop::Bowl), propBox(*layout, Prop::Ball)));
        EXPECT_FALSE(overlap(
            propBox(*layout, Prop::Ball), propBox(*layout, Prop::Nest)));
        EXPECT_FALSE(overlap(
            propBox(*layout, Prop::Bowl), propBox(*layout, Prop::Nest)));
    }

    TEST(PetLayoutTest, PropAt_IsHalfOpenAtEveryEdge)
    {
        const auto layout = layoutFor(kCanvas);
        ASSERT_TRUE(layout.has_value());

        const Rect bowl = propBox(*layout, Prop::Bowl);

        EXPECT_TRUE(propAt(kCanvas, bowl.origin).has_value());
        EXPECT_FALSE(propAt(
                         kCanvas,
                         Point{
                             .x = bowl.origin.x
                                  + static_cast<std::int32_t>(
                                      bowl.size.width),
                             .y = bowl.origin.y})
                         .has_value());
        EXPECT_FALSE(propAt(
                         kCanvas,
                         Point{
                             .x = bowl.origin.x,
                             .y = bowl.origin.y
                                  + static_cast<std::int32_t>(
                                      bowl.size.height)})
                         .has_value());
    }

    TEST(PetLayoutTest, PropAt_AnswersNothingWhereNoPropIs)
    {
        EXPECT_FALSE(propAt(kCanvas, Point{.x = 128, .y = 64}));
    }

    // A window with no room for the grid has no props to press either.
    TEST(PetLayoutTest, PropAt_AnswersNothingWithoutAGrid)
    {
        EXPECT_FALSE(propAt(
            Size{.width = 16, .height = 16}, Point{.x = 8, .y = 8}));
    }
} // namespace
