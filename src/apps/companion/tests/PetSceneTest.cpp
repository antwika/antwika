#include <algorithm>
#include <array>
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
#include <antwika/gfx/TextLayout.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/i18n/Translator.hpp>

#include "antwika/companion/PetLayout.hpp"
#include "antwika/companion/PetScene.hpp"
#include "antwika/companion/PetSnapshot.hpp"
#include "antwika/companion/Saying.hpp"

using antwika::companion::kSceneUnits;
using antwika::companion::PetScene;
using antwika::companion::PetSnapshot;
using antwika::companion::PetState;
using antwika::companion::Saying;
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

    // The bubble and its tail are the last two rectangles drawn.
    // They go in after the animal, so a bubble is never behind it.
    [[nodiscard]] Rect bubbleOf(const Drawn &drawn)
    {
        return drawn.rects[drawn.rects.size() - 2];
    }

    // Every line the companion has.
    // So a new one cannot be added without a test that draws it.
    constexpr std::array<Saying, 9> kEveryLine{
        Saying::Hello,
        Saying::Bored,
        Saying::NiceDay,
        Saying::Silly,
        Saying::FeedMe,
        Saying::Yum,
        Saying::NotHungry,
        Saying::LetMeSleep,
        Saying::Zzz};

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
        const antwika::i18n::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};
        const Drawn drawn =
            render(scene, {.width = 8, .height = 8}, awake());

        EXPECT_TRUE(drawn.rects.empty());
        EXPECT_TRUE(drawn.texts.empty());
    }

    TEST(PetSceneTest, TheSquarePictureIsCentredOnWhicheverSideIsLonger)
    {
        const antwika::i18n::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

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
        const antwika::i18n::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};
        const Drawn drawn = render(scene, kCanvas, awake());

        EXPECT_EQ(drawn.rects.size(), kBareAwakeRects);
    }

    TEST(PetSceneTest, NightIsADifferentPictureFromDay)
    {
        const antwika::i18n::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

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
        const antwika::i18n::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

        PetSnapshot hungry = awake();
        hungry.hungry = true;
        hungry.hunger = 4;

        const Drawn drawn = render(scene, kCanvas, hungry);

        // The bowl and its rim, plus the hunger gauge's fill.
        EXPECT_EQ(drawn.rects.size(), kBareAwakeRects + 3);
    }

    TEST(PetSceneTest, ASleepingCompanionShutsItsEyesAndPuffs)
    {
        const antwika::i18n::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

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
        const antwika::i18n::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

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
        const antwika::i18n::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

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
        const antwika::i18n::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

        PetSnapshot gone = awake();
        gone.state = PetState::Perished;
        gone.happiness = 0;

        const Drawn drawn = render(scene, kCanvas, gone);
        const Drawn alive = render(scene, kCanvas, awake());

        EXPECT_NE(drawn.cleared, alive.cleared);

        // The ground, the sun and two gauge backgrounds with no fill.
        // Plus the four boxes of a grave and the one button over it.
        EXPECT_EQ(drawn.rects.size(), 4U + 4U + 1U);
    }

    // The one thing on screen that is pressed rather than read.
    // It is painted into the very box ReviveSink hit-tests.
    // Shared, so the two cannot drift apart.
    TEST(PetSceneTest, Draw_OffersANewCompanionOnceItHasPerished)
    {
        const antwika::i18n::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

        PetSnapshot gone = awake();
        gone.state = PetState::Perished;
        gone.happiness = 0;

        const Drawn drawn = render(scene, kCanvas, gone);
        const auto button =
            antwika::companion::reviveButtonRect(kCanvas);

        ASSERT_TRUE(button.has_value());
        EXPECT_NE(
            std::find(drawn.rects.begin(), drawn.rects.end(), *button),
            drawn.rects.end());

        std::vector<std::string> words;
        for (const auto &text : drawn.texts)
        {
            words.push_back(text.text);
        }

        EXPECT_NE(
            std::find(words.begin(), words.end(), "new pet"),
            words.end());
    }

    // A living companion is offered nothing.
    // The button means the one thing there is left to do.
    TEST(PetSceneTest, Draw_OffersNoButtonWhileTheCompanionIsAlive)
    {
        const antwika::i18n::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

        const Drawn drawn = render(scene, kCanvas, awake());
        const auto button =
            antwika::companion::reviveButtonRect(kCanvas);

        ASSERT_TRUE(button.has_value());
        EXPECT_EQ(
            std::find(drawn.rects.begin(), drawn.rects.end(), *button),
            drawn.rects.end());
    }

    TEST(PetSceneTest, AnEmptyGaugeDrawsOnlyItsBackground)
    {
        const antwika::i18n::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

        PetSnapshot flat = awake();
        flat.happiness = 0;

        const Drawn drawn = render(scene, kCanvas, flat);

        EXPECT_EQ(drawn.rects.size(), kBareAwakeRects - 1);
    }

    TEST(PetSceneTest, AGaugeWithNoMaximumDrawsOnlyItsBackground)
    {
        const antwika::i18n::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

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
        const antwika::i18n::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

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
        const antwika::i18n::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

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
        const antwika::i18n::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

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
        const antwika::i18n::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

        PetSnapshot gone = awake();
        gone.state = PetState::Perished;
        gone.happiness = 0;

        const Drawn drawn = render(scene, kCanvas, gone);

        // The button's own word first, then the three of the readout.
        // The readout is drawn last, so nothing sits on top of it.
        ASSERT_EQ(drawn.texts.size(), kReadoutLines + 1);
        EXPECT_EQ(drawn.texts[2].text, "happy 0/10");
        EXPECT_NE(
            drawn.texts[1].color,
            render(scene, kCanvas, awake()).texts[0].color);
    }

    // Four glyph pixels to a unit, and a unit scales with the canvas.
    // So the two readouts differ by exactly what the windows do.
    TEST(PetSceneTest, Draw_ScalesTheReadoutWithTheWindow)
    {
        const antwika::i18n::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

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
        const antwika::i18n::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};
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
        const antwika::i18n::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};
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

    TEST(PetSceneTest, Draw_DrawsNoBubbleWhileThereIsNothingToSay)
    {
        const antwika::i18n::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};
        const Drawn drawn = render(scene, kCanvas, awake());

        EXPECT_EQ(drawn.rects.size(), kBareAwakeRects);
        EXPECT_EQ(drawn.texts.size(), kReadoutLines);
    }

    // The bubble, its tail, and one line of text ahead of the readout.
    TEST(PetSceneTest, Draw_PutsWhatItSaysInABubbleBesideTheAnimal)
    {
        const antwika::i18n::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

        PetSnapshot talking = awake();
        talking.saying = Saying::FeedMe;

        const Drawn drawn = render(scene, kCanvas, talking);

        EXPECT_EQ(drawn.rects.size(), kBareAwakeRects + 2);
        ASSERT_EQ(drawn.texts.size(), kReadoutLines + 1);
        EXPECT_EQ(drawn.texts[0].text, "feed me!");
    }

    // A different line is a different bubble, and the same one is not.
    TEST(PetSceneTest, Draw_SaysADifferentThingForADifferentLine)
    {
        const antwika::i18n::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

        PetSnapshot bored = awake();
        bored.saying = Saying::Bored;

        PetSnapshot asking = awake();
        asking.saying = Saying::FeedMe;

        EXPECT_NE(
            render(scene, kCanvas, bored).texts[0].text,
            render(scene, kCanvas, asking).texts[0].text);
        EXPECT_EQ(
            render(scene, kCanvas, bored).texts[0].text,
            render(scene, kCanvas, bored).texts[0].text);
    }

    // The words are scaled to the longest line rather than to each one.
    // Which is worth nothing unless the longest one actually fits.
    TEST(PetSceneTest, Draw_KeepsEveryLineInsideItsOwnBubble)
    {
        const antwika::i18n::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

        for (const Saying line : kEveryLine)
        {
            PetSnapshot talking = awake();
            talking.saying = line;

            const Drawn drawn = render(scene, kCanvas, talking);
            const Rect bubble = bubbleOf(drawn);
            const Text &text = drawn.texts[0];
            const Size size =
                antwika::gfx::textSize(text.text, text.scale);

            EXPECT_FALSE(text.text.empty());
            EXPECT_GE(text.origin.x, bubble.origin.x);
            EXPECT_LE(
                text.origin.x + static_cast<std::int32_t>(size.width),
                bubble.origin.x
                    + static_cast<std::int32_t>(bubble.size.width));
            EXPECT_GE(text.origin.y, bubble.origin.y);
            EXPECT_LE(
                text.origin.y + static_cast<std::int32_t>(size.height),
                bubble.origin.y
                    + static_cast<std::int32_t>(bubble.size.height));
        }
    }

    // The bubble covers neither gauge, neither the bowl nor the ground.
    // A bubble hiding what it is being said about is worse than none.
    TEST(PetSceneTest, Draw_KeepsTheBubbleClearOfTheGauges)
    {
        const antwika::i18n::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

        PetSnapshot talking = awake();
        talking.saying = Saying::Hello;

        const Drawn drawn = render(scene, kCanvas, talking);
        const Rect bubble = bubbleOf(drawn);

        // The happiness gauge, the lower of the two.
        const Rect gauge = drawn.rects[3];
        const Rect ground = drawn.rects[0];

        EXPECT_GE(
            bubble.origin.y,
            gauge.origin.y + static_cast<std::int32_t>(gauge.size.height));
        EXPECT_LE(
            bubble.origin.y
                + static_cast<std::int32_t>(bubble.size.height),
            ground.origin.y);
    }

    // Four bubble pixels to a glyph pixel at the shipped window.
    // So the words double when the window does, like the readout.
    TEST(PetSceneTest, Draw_ScalesTheBubbleTextWithTheWindow)
    {
        const antwika::i18n::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

        PetSnapshot talking = awake();
        talking.saying = Saying::NiceDay;

        const Drawn small = render(
            scene, {.width = 128, .height = 128}, talking);
        const Drawn shipped = render(scene, kCanvas, talking);
        const Drawn large = render(
            scene, {.width = 512, .height = 512}, talking);

        // A unit too small for a scaled glyph still gets the smallest.
        EXPECT_EQ(small.texts[0].scale, 1U);
        EXPECT_EQ(shipped.texts[0].scale, 1U);
        EXPECT_EQ(large.texts[0].scale, 2U);
    }

    // The window is a whole number of pixels to the unit.
    // Which main.cpp derives its size from rather than restates.
    TEST(PetSceneTest, Draw_GivesEveryUnitAWholeNumberOfPixels)
    {
        const antwika::i18n::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};
        const Drawn drawn = render(scene, kCanvas, awake());

        ASSERT_FALSE(drawn.rects.empty());

        // The ground, which spans the whole grid.
        EXPECT_EQ(drawn.rects[0].size.width, kCanvas.width);
        EXPECT_EQ(kCanvas.width % kSceneUnits, 0U);
    }

    // Every word drawn here comes off the injected translator.
    // Which is the whole of what adopting antwika::i18n bought.
    // The readout, the bubble and the button all change together.
    // Nothing in the snapshot differs between the two renders.
    // So a language cannot reach anything a replay reproduces.
    TEST(PetSceneTest, Draw_SaysEveryWordInTheTranslatorsLanguage)
    {
        const antwika::i18n::Translator swedish{
            antwika::i18n::Locale::Swedish};
        const PetScene scene{swedish};

        PetSnapshot talking = awake();
        talking.hungry = true;
        talking.saying = Saying::FeedMe;

        const Drawn drawn = render(scene, kCanvas, talking);

        ASSERT_EQ(drawn.texts.size(), kReadoutLines + 1);
        EXPECT_EQ(drawn.texts[0].text, "mata mig!");
        EXPECT_EQ(drawn.texts[1].text, "hunger 0/8");
        EXPECT_EQ(drawn.texts[2].text, "glad 6/10");
        EXPECT_EQ(drawn.texts[3].text, "vaken, hungrig");
    }

    // The one button a perished companion is offered says it too.
    TEST(PetSceneTest, Draw_WordsTheNewCompanionButtonAsWell)
    {
        const antwika::i18n::Translator swedish{
            antwika::i18n::Locale::Swedish};
        const PetScene scene{swedish};

        PetSnapshot gone = awake();
        gone.state = PetState::Perished;

        const Drawn drawn = render(scene, kCanvas, gone);

        EXPECT_EQ(drawn.texts[0].text, "nytt djur");
        EXPECT_EQ(lastLine(drawn), "borta");
    }

    // The bubble is scaled to the longest line the catalogue holds.
    // A count written into the scene would be the English one.
    // The longest Swedish line is half again the longest English one.
    // So the same window gives its words a smaller scale.
    TEST(PetSceneTest, Draw_ScalesTheBubbleToTheLongestLineInUse)
    {
        const antwika::i18n::Translator english{
            antwika::i18n::Locale::English};
        const antwika::i18n::Translator swedish{
            antwika::i18n::Locale::Swedish};

        PetSnapshot talking = awake();
        talking.saying = Saying::Hello;

        const Drawn drawn = render(
            PetScene{english}, {.width = 512, .height = 512}, talking);
        const Drawn other = render(
            PetScene{swedish}, {.width = 512, .height = 512}, talking);

        EXPECT_GT(drawn.texts[0].scale, other.texts[0].scale);
    }
} // namespace
