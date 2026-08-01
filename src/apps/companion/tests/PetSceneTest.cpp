#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Glyphs.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/companion/PetScene.hpp"
#include "antwika/companion/PetSnapshot.hpp"

using antwika::companion::kSceneUnits;
using antwika::companion::PetScene;
using antwika::companion::PetSnapshot;
using antwika::companion::PetState;
using antwika::gfx::Color;
using antwika::gfx::kGlyphLineHeight;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::Point;
using antwika::gfx::Rect;
using antwika::gfx::Size;
using ::testing::_;
using ::testing::NiceMock;

namespace
{
    // 256 pixels square is what main.cpp asks for.
    // 32 whole units a side divides into it exactly eight pixels each.
    constexpr Size kCanvas{.width = 256, .height = 256};

    // The ground, the sun, both gauge backgrounds, one gauge fill.
    // Plus the ten boxes an animal is made of.
    constexpr std::size_t kBareAwakeRects = 15;

    // Hunger, happiness, and what the companion is doing.
    constexpr std::size_t kReadoutLines = 3;

    struct Text
    {
        Point origin{};
        std::string text;
        std::uint32_t scale = 0;
        Color color{};
    };

    struct Drawn
    {
        Color cleared{};
        std::vector<Rect> rects;
        std::vector<Color> colors;
        std::vector<Text> texts;
    };

    Drawn render(
        const PetScene &scene,
        const Size canvas,
        const PetSnapshot &snapshot)
    {
        NiceMock<MockRenderer> renderer;
        Drawn drawn;

        ON_CALL(renderer, clear(_))
            .WillByDefault([&drawn](const Color color)
                           { drawn.cleared = color; });
        ON_CALL(renderer, drawRect(_, _))
            .WillByDefault(
                [&drawn](const Rect rect, const Color color)
                {
                    drawn.rects.push_back(rect);
                    drawn.colors.push_back(color);
                });
        ON_CALL(renderer, drawText(_, _, _, _))
            .WillByDefault(
                [&drawn](
                    const Point origin,
                    const std::string_view text,
                    const std::uint32_t scale,
                    const Color color)
                {
                    drawn.texts.push_back(
                        Text{
                            .origin = origin,
                            .text = std::string(text),
                            .scale = scale,
                            .color = color});
                });

        scene.draw(renderer, canvas, snapshot);
        return drawn;
    }

    [[nodiscard]] std::string lastLine(const Drawn &drawn)
    {
        return drawn.texts.back().text;
    }

    PetSnapshot awake()
    {
        return PetSnapshot{
            .state = PetState::Awake,
            .night = false,
            .hungry = false,
            .disturbed = false,
            .hunger = 0,
            .hungerMax = 8,
            .happiness = 6,
            .happinessMax = 10,
            .ticks = 0};
    }

    TEST(PetSceneTest, ACanvasTooSmallForAUnitDrawsTheSkyAndStops)
    {
        const PetScene scene;
        const Drawn drawn =
            render(scene, {.width = 8, .height = 8}, awake());

        EXPECT_TRUE(drawn.rects.empty());
        EXPECT_TRUE(drawn.texts.empty());
    }

    TEST(PetSceneTest, TheSquarePictureIsCentredOnWhicheverSideIsLonger)
    {
        const PetScene scene;

        // Four pixels a unit either way.
        // So the two differ only in which side is left over.
        const Drawn tall =
            render(scene, {.width = 128, .height = 256}, awake());
        const Drawn wide =
            render(scene, {.width = 256, .height = 128}, awake());

        ASSERT_FALSE(tall.rects.empty());
        ASSERT_FALSE(wide.rects.empty());

        // The ground, which starts at the left edge of the grid.
        EXPECT_EQ(tall.rects[0].origin.x, 0);
        EXPECT_EQ(tall.rects[0].origin.y, 64 + 96);
        EXPECT_EQ(wide.rects[0].origin.x, 64);
        EXPECT_EQ(wide.rects[0].origin.y, 96);
    }

    TEST(PetSceneTest, AnUnhungryAwakeCompanionIsTheBarePicture)
    {
        const PetScene scene;
        const Drawn drawn = render(scene, kCanvas, awake());

        EXPECT_EQ(drawn.rects.size(), kBareAwakeRects);
    }

    TEST(PetSceneTest, NightIsADifferentPictureFromDay)
    {
        const PetScene scene;

        PetSnapshot night = awake();
        night.night = true;

        const Drawn day = render(scene, kCanvas, awake());
        const Drawn dark = render(scene, kCanvas, night);

        EXPECT_NE(day.cleared, dark.cleared);
        EXPECT_EQ(day.rects, dark.rects);
        EXPECT_NE(day.colors, dark.colors);
    }

    TEST(PetSceneTest, AHungryCompanionIsShownItsEmptyBowl)
    {
        const PetScene scene;

        PetSnapshot hungry = awake();
        hungry.hungry = true;
        hungry.hunger = 4;

        const Drawn drawn = render(scene, kCanvas, hungry);

        // The bowl and its rim, plus the hunger gauge's fill.
        EXPECT_EQ(drawn.rects.size(), kBareAwakeRects + 3);
    }

    TEST(PetSceneTest, ASleepingCompanionShutsItsEyesAndPuffs)
    {
        const PetScene scene;

        PetSnapshot asleep = awake();
        asleep.state = PetState::Asleep;
        asleep.night = true;

        const Drawn drawn = render(scene, kCanvas, asleep);

        // One puff on the first frame of the drowse clip.
        EXPECT_EQ(drawn.rects.size(), kBareAwakeRects + 1);
        EXPECT_NE(drawn.rects, render(scene, kCanvas, awake()).rects);
    }

    TEST(PetSceneTest, TheDrowseClipAddsAPuffPerFrame)
    {
        const PetScene scene;

        PetSnapshot asleep = awake();
        asleep.state = PetState::Asleep;
        asleep.night = true;

        // Three quarters of a second a frame, at kTicksPerSecond.
        asleep.ticks = 2 * (3 * antwika::companion::kTicksPerSecond / 4);
        const Drawn drawn = render(scene, kCanvas, asleep);

        EXPECT_EQ(drawn.rects.size(), kBareAwakeRects + 3);
    }

    // The eyes and the bob resolve from the tick count alone.
    // So the same tick is always the same picture.
    // And a different tick may well not be.
    TEST(PetSceneTest, TheIdleAnimationIsAFunctionOfTheTickCount)
    {
        const PetScene scene;

        PetSnapshot blinking = awake();
        blinking.ticks = 3 * antwika::companion::kTicksPerSecond + 1;

        PetSnapshot bobbing = awake();
        bobbing.ticks = antwika::companion::kTicksPerSecond / 2;

        const Drawn open = render(scene, kCanvas, awake());
        const Drawn shut = render(scene, kCanvas, blinking);
        const Drawn bobbed = render(scene, kCanvas, bobbing);

        EXPECT_NE(open.rects, shut.rects);
        EXPECT_NE(open.rects, bobbed.rects);
        EXPECT_EQ(open.rects, render(scene, kCanvas, awake()).rects);
    }

    TEST(PetSceneTest, APerishedCompanionGetsAGraveAndItsOwnPalette)
    {
        const PetScene scene;

        PetSnapshot gone = awake();
        gone.state = PetState::Perished;
        gone.happiness = 0;

        const Drawn drawn = render(scene, kCanvas, gone);
        const Drawn alive = render(scene, kCanvas, awake());

        EXPECT_NE(drawn.cleared, alive.cleared);

        // The ground, the sun and two gauge backgrounds with no fill.
        // Plus the four boxes of a grave.
        EXPECT_EQ(drawn.rects.size(), 4U + 4U);
    }

    TEST(PetSceneTest, AnEmptyGaugeDrawsOnlyItsBackground)
    {
        const PetScene scene;

        PetSnapshot flat = awake();
        flat.happiness = 0;

        const Drawn drawn = render(scene, kCanvas, flat);

        EXPECT_EQ(drawn.rects.size(), kBareAwakeRects - 1);
    }

    TEST(PetSceneTest, AGaugeWithNoMaximumDrawsOnlyItsBackground)
    {
        const PetScene scene;

        PetSnapshot unscaled = awake();
        unscaled.hungerMax = 0;
        unscaled.happinessMax = 0;
        unscaled.hunger = 3;
        unscaled.happiness = 3;

        const Drawn drawn = render(scene, kCanvas, unscaled);

        EXPECT_EQ(drawn.rects.size(), kBareAwakeRects - 1);
    }

    TEST(PetSceneTest, AGaugeNeverFillsPastItsOwnWidth)
    {
        const PetScene scene;

        PetSnapshot brimming = awake();
        brimming.happiness = 99;
        brimming.happinessMax = 10;

        const Drawn drawn = render(scene, kCanvas, brimming);

        PetSnapshot full = awake();
        full.happiness = 10;
        full.happinessMax = 10;

        EXPECT_EQ(drawn.rects, render(scene, kCanvas, full).rects);
    }

    TEST(PetSceneTest, Draw_ReportsBothGaugesAndTheStateInWords)
    {
        const PetScene scene;

        PetSnapshot fed = awake();
        fed.hunger = 3;

        const Drawn drawn = render(scene, kCanvas, fed);

        ASSERT_EQ(drawn.texts.size(), kReadoutLines);
        EXPECT_EQ(drawn.texts[0].text, "hunger 3/8");
        EXPECT_EQ(drawn.texts[1].text, "happy 6/10");
        EXPECT_EQ(drawn.texts[2].text, "awake");
    }

    TEST(PetSceneTest, Draw_SaysWhichOfItsStatesTheCompanionIsIn)
    {
        const PetScene scene;

        PetSnapshot hungry = awake();
        hungry.hungry = true;

        PetSnapshot asleep = awake();
        asleep.state = PetState::Asleep;
        asleep.night = true;

        PetSnapshot woken = asleep;
        woken.disturbed = true;

        PetSnapshot gone = awake();
        gone.state = PetState::Perished;
        gone.happiness = 0;

        EXPECT_EQ(lastLine(render(scene, kCanvas, awake())), "awake");
        EXPECT_EQ(
            lastLine(render(scene, kCanvas, hungry)), "awake, hungry");
        EXPECT_EQ(lastLine(render(scene, kCanvas, asleep)), "asleep");
        EXPECT_EQ(
            lastLine(render(scene, kCanvas, woken)), "asleep, woken");
        EXPECT_EQ(lastLine(render(scene, kCanvas, gone)), "gone");
    }

    // A perished companion is exactly what somebody wants reported.
    // So the readout outlived the early return the grave used to be.
    TEST(PetSceneTest, Draw_ReportsAPerishedCompanionToo)
    {
        const PetScene scene;

        PetSnapshot gone = awake();
        gone.state = PetState::Perished;
        gone.happiness = 0;

        const Drawn drawn = render(scene, kCanvas, gone);

        ASSERT_EQ(drawn.texts.size(), kReadoutLines);
        EXPECT_EQ(drawn.texts[1].text, "happy 0/10");
        EXPECT_NE(
            drawn.texts[0].color,
            render(scene, kCanvas, awake()).texts[0].color);
    }

    // Four glyph pixels to a unit, and a unit scales with the canvas.
    // So the two readouts differ by exactly what the windows do.
    TEST(PetSceneTest, Draw_ScalesTheReadoutWithTheWindow)
    {
        const PetScene scene;

        const Drawn small =
            render(scene, {.width = 128, .height = 128}, awake());
        const Drawn large = render(scene, kCanvas, awake());

        ASSERT_EQ(small.texts.size(), kReadoutLines);
        ASSERT_EQ(large.texts.size(), kReadoutLines);
        EXPECT_EQ(small.texts[0].scale, 1U);
        EXPECT_EQ(large.texts[0].scale, 2U);
    }

    // A unit too small for a scaled glyph still gets the smallest text.
    // And the readout still stands on the grid rather than under it.
    // Because it is anchored to the bottom rather than to a row.
    TEST(PetSceneTest, Draw_KeepsTheSmallestReadoutOnTheGrid)
    {
        const PetScene scene;
        const Size canvas{.width = 64, .height = 64};

        const Drawn drawn = render(scene, canvas, awake());

        ASSERT_EQ(drawn.texts.size(), kReadoutLines);
        EXPECT_EQ(drawn.texts[0].scale, 1U);

        const auto bottom =
            drawn.texts.back().origin.y
            + static_cast<std::int32_t>(kGlyphLineHeight);
        EXPECT_LE(bottom, static_cast<std::int32_t>(canvas.height));
        EXPECT_GT(drawn.texts[0].origin.y, 0);
    }

    // Three lines, one line height apart, in declaration order.
    TEST(PetSceneTest, Draw_StacksTheReadoutOneLineHeightApart)
    {
        const PetScene scene;
        const Drawn drawn = render(scene, kCanvas, awake());

        ASSERT_EQ(drawn.texts.size(), kReadoutLines);

        const auto step = static_cast<std::int32_t>(
            kGlyphLineHeight * drawn.texts[0].scale);

        EXPECT_EQ(drawn.texts[0].origin.x, drawn.texts[1].origin.x);
        EXPECT_EQ(drawn.texts[1].origin.x, drawn.texts[2].origin.x);
        EXPECT_EQ(
            drawn.texts[1].origin.y, drawn.texts[0].origin.y + step);
        EXPECT_EQ(
            drawn.texts[2].origin.y, drawn.texts[1].origin.y + step);
    }

    // The window is a whole number of pixels to the unit.
    // Which main.cpp derives its size from rather than restates.
    TEST(PetSceneTest, Draw_GivesEveryUnitAWholeNumberOfPixels)
    {
        const PetScene scene;
        const Drawn drawn = render(scene, kCanvas, awake());

        ASSERT_FALSE(drawn.rects.empty());

        // The ground, which spans the whole grid.
        EXPECT_EQ(drawn.rects[0].size.width, kCanvas.width);
        EXPECT_EQ(kCanvas.width % kSceneUnits, 0U);
    }
} // namespace
