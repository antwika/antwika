#include <cstddef>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/companion/PetScene.hpp"
#include "antwika/companion/PetSnapshot.hpp"

using antwika::companion::PetScene;
using antwika::companion::PetSnapshot;
using antwika::companion::PetState;
using antwika::gfx::Color;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::Rect;
using antwika::gfx::Size;
using ::testing::_;
using ::testing::NiceMock;

namespace
{
    // 128 pixels square is what main.cpp asks for.
    // 32 whole units a side divides into it exactly four pixels each.
    constexpr Size kCanvas{.width = 128, .height = 128};

    // The ground, the sun, both gauge backgrounds, one gauge fill.
    // Plus the ten boxes an animal is made of.
    constexpr std::size_t kBareAwakeRects = 15;

    struct Drawn
    {
        Color cleared{};
        std::vector<Rect> rects;
        std::vector<Color> colors;
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

        scene.draw(renderer, canvas, snapshot);
        return drawn;
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
} // namespace
