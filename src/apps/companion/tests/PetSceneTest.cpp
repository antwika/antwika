#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <set>
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

#include "antwika/companion/DayMood.hpp"
#include "antwika/companion/LifeStage.hpp"
#include <antwika/time/Tick.hpp>

#include "antwika/companion/PetLayout.hpp"
#include "antwika/companion/PetScene.hpp"
#include "antwika/companion/PetSnapshot.hpp"
#include "antwika/companion/Saying.hpp"

using antwika::companion::DayMood;
using antwika::companion::kSceneUnits;
using antwika::companion::layoutFor;
using antwika::companion::LifeStage;
using antwika::companion::LineageMemory;
using antwika::companion::PetForm;
using antwika::companion::PetScene;
using antwika::companion::PetSnapshot;
using antwika::companion::PetState;
using antwika::companion::Prop;
using antwika::companion::propBox;
using antwika::companion::reviveButtonRect;
using antwika::companion::Saying;
using antwika::gfx::Color;
using antwika::gfx::kGlyphLineHeight;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::Point;
using antwika::gfx::Rect;
using antwika::gfx::Size;
using antwika::time::Tick;
using ::testing::_;
using ::testing::NiceMock;

namespace
{
    // 256 pixels square is what main.cpp asks for.
    // 32 whole units a side divides into it exactly eight pixels each.
    constexpr Size kCanvas{.width = 256, .height = 256};

    // The ground and the sun.
    // Four gauge tracks, three of them filled.
    // Three props of two rectangles each.
    // And the ten boxes an animal is made of.
    constexpr std::size_t kBareAwakeRects = 25;

    // What the companion is doing, how old the day is, and the record.
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

    // The readout is drawn last and is three lines.
    // So the state is three from the end, whatever came before it.
    [[nodiscard]] std::string stateLine(const Drawn &drawn)
    {
        return drawn.texts[drawn.texts.size() - kReadoutLines].text;
    }

    [[nodiscard]] std::string dayLine(const Drawn &drawn)
    {
        return drawn.texts[drawn.texts.size() - 2].text;
    }

    [[nodiscard]] std::string lineageLine(const Drawn &drawn)
    {
        return drawn.texts.back().text;
    }

    // The bubble and its tail are the last two rectangles drawn.
    // They go in after the animal, so a bubble is never behind it.
    [[nodiscard]] Rect bubbleOf(const Drawn &drawn)
    {
        return drawn.rects[drawn.rects.size() - 2];
    }

    [[nodiscard]] bool drew(const Drawn &drawn, const Rect &rect)
    {
        return std::find(drawn.rects.begin(), drawn.rects.end(), rect)
               != drawn.rects.end();
    }

    [[nodiscard]] Color colorOf(const Drawn &drawn, const Rect &rect)
    {
        const auto found =
            std::find(drawn.rects.begin(), drawn.rects.end(), rect);

        return drawn.colors[static_cast<std::size_t>(
            found - drawn.rects.begin())];
    }

    // Every line the companion has.
    // So a new one cannot be added without a test that draws it.
    constexpr std::array<Saying, 15> kEveryLine{
        Saying::Hello,
        Saying::Bored,
        Saying::NiceDay,
        Saying::Silly,
        Saying::FeedMe,
        Saying::Yum,
        Saying::NotHungry,
        Saying::LetMeSleep,
        Saying::Zzz,
        Saying::PlayWithMe,
        Saying::Wheee,
        Saying::TooTired,
        Saying::NotSleepy,
        Saying::Yawn,
        Saying::Poked};

    constexpr std::array<Prop, 3> kProps{
        Prop::Bowl, Prop::Ball, Prop::Nest};

    PetSnapshot awake()
    {
        return PetSnapshot{
            .state = PetState::Awake,
            .asleep = false,
            .hungry = false,
            .bored = false,
            .tired = false,
            .disturbed = false,
            .saying = Saying::None,
            .hunger = 0,
            .hungerMax = 8,
            .fun = 10,
            .funMax = 10,
            .happiness = 6,
            .happinessMax = 10,
            .energy = 30,
            .energyCeiling = 30,
            .ticks = 0,
            .day = 0,
            .mood = DayMood::Ordinary,
            .stage = LifeStage::Egg,
            .form = PetForm::Plain,
            .lineage = LineageMemory{}};
    }

    PetSnapshot asleep()
    {
        PetSnapshot snapshot = awake();
        snapshot.state = PetState::Asleep;
        snapshot.asleep = true;
        snapshot.tired = true;
        return snapshot;
    }

    PetSnapshot perished()
    {
        PetSnapshot snapshot = awake();
        snapshot.state = PetState::Perished;
        snapshot.energy = 0;
        snapshot.energyCeiling = 0;
        return snapshot;
    }

    TEST(PetSceneTest, ACanvasTooSmallForAUnitDrawsTheSkyAndStops)
    {
        const PetScene scene;

        const Drawn drawn =
            render(scene, Size{.width = 16, .height = 16}, awake());

        EXPECT_TRUE(drawn.rects.empty());
        EXPECT_TRUE(drawn.texts.empty());
    }

    TEST(PetSceneTest, TheSquarePictureIsCentredOnWhicheverSideIsLonger)
    {
        const PetScene scene;

        const Drawn drawn =
            render(scene, Size{.width = 320, .height = 256}, awake());

        ASSERT_FALSE(drawn.rects.empty());
        EXPECT_EQ(drawn.rects[0].origin.x, 32);
        EXPECT_EQ(drawn.rects[0].origin.y, 176);
    }

    TEST(PetSceneTest, AnUnhungryAwakeCompanionIsTheBarePicture)
    {
        const PetScene scene;

        const Drawn drawn = render(scene, kCanvas, awake());

        EXPECT_EQ(drawn.rects.size(), kBareAwakeRects);
        EXPECT_EQ(drawn.texts.size(), kReadoutLines);
    }

    TEST(PetSceneTest, NightIsADifferentPictureFromDay)
    {
        const PetScene scene;

        const Drawn day = render(scene, kCanvas, awake());
        const Drawn night = render(scene, kCanvas, asleep());

        EXPECT_NE(day.cleared, night.cleared);
    }

    // The three props are painted into the boxes propAt() hit-tests.
    // So aiming at one and hitting it are one rectangle.
    TEST(PetSceneTest, EveryPropIsPaintedIntoItsOwnHitBox)
    {
        const PetScene scene;
        const auto layout = layoutFor(kCanvas);
        ASSERT_TRUE(layout.has_value());

        const Drawn drawn = render(scene, kCanvas, awake());

        for (const Prop prop : kProps)
        {
            EXPECT_TRUE(drew(drawn, propBox(*layout, prop)));
        }
    }

    // The one the companion would like is lit rather than present.
    // Which is this application's whole answer to instructions.
    TEST(PetSceneTest, TheWantedPropIsLitAndTheOthersAreNot)
    {
        const PetScene scene;
        const auto layout = layoutFor(kCanvas);
        ASSERT_TRUE(layout.has_value());

        const auto innerOf = [&layout](const Drawn &drawn, const Prop p)
        {
            const Rect area = propBox(*layout, p);
            const auto unit = static_cast<std::int32_t>(layout->unit);
            const Rect inner{
                .origin =
                    {.x = area.origin.x + unit, .y = area.origin.y + unit},
                .size = {
                    .width = area.size.width - 2 * layout->unit,
                    .height = area.size.height - 2 * layout->unit}};
            return colorOf(drawn, inner);
        };

        PetSnapshot hungry = awake();
        hungry.hungry = true;

        const Drawn plain = render(scene, kCanvas, awake());
        const Drawn wanting = render(scene, kCanvas, hungry);

        EXPECT_NE(
            innerOf(plain, Prop::Bowl), innerOf(wanting, Prop::Bowl));
        EXPECT_EQ(
            innerOf(plain, Prop::Ball), innerOf(wanting, Prop::Ball));
        EXPECT_EQ(
            innerOf(plain, Prop::Nest), innerOf(wanting, Prop::Nest));
    }

    TEST(PetSceneTest, EachNeedLightsItsOwnProp)
    {
        const PetScene scene;

        PetSnapshot bored = awake();
        bored.bored = true;
        PetSnapshot tired = awake();
        tired.tired = true;

        // Three snapshots differing in one flag draw three pictures.
        // So no prop ever answers another prop's need.
        EXPECT_NE(
            render(scene, kCanvas, awake()).colors,
            render(scene, kCanvas, bored).colors);
        EXPECT_NE(
            render(scene, kCanvas, awake()).colors,
            render(scene, kCanvas, tired).colors);
        EXPECT_NE(
            render(scene, kCanvas, bored).colors,
            render(scene, kCanvas, tired).colors);
    }

    // What it grew into is a shade of the one fur a palette holds.
    // Rather than three more colours in every palette.
    TEST(PetSceneTest, TheFormItGrewIntoShadesTheFur)
    {
        const PetScene scene;

        PetSnapshot bright = awake();
        bright.form = PetForm::Bright;
        PetSnapshot scruffy = awake();
        scruffy.form = PetForm::Scruffy;

        const Drawn plain = render(scene, kCanvas, awake());
        const Drawn shiny = render(scene, kCanvas, bright);
        const Drawn shabby = render(scene, kCanvas, scruffy);

        EXPECT_NE(plain.colors, shiny.colors);
        EXPECT_NE(plain.colors, shabby.colors);
        EXPECT_NE(shiny.colors, shabby.colors);
    }

    TEST(PetSceneTest, ASleepingCompanionShutsItsEyesAndPuffs)
    {
        const PetScene scene;

        const Drawn up = render(scene, kCanvas, awake());
        const Drawn down = render(scene, kCanvas, asleep());

        EXPECT_GT(down.rects.size(), up.rects.size());
    }

    TEST(PetSceneTest, TheDrowseClipAddsAPuffPerFrame)
    {
        const PetScene scene;

        PetSnapshot first = asleep();
        PetSnapshot later = asleep();
        later.ticks = 3 * antwika::companion::kTicksPerSecond / 4;

        const Drawn one = render(scene, kCanvas, first);
        const Drawn two = render(scene, kCanvas, later);

        EXPECT_EQ(two.rects.size(), one.rects.size() + 1);
    }

    // An awake companion shuts its eyes to blink as well as to sleep.
    // So the two ways of arriving at shut eyes are both drawn.
    TEST(PetSceneTest, AnAwakeCompanionBlinksOnItsOwn)
    {
        const PetScene scene;

        PetSnapshot open = awake();
        PetSnapshot blinking = awake();
        blinking.ticks = 3 * antwika::companion::kTicksPerSecond + 1;

        EXPECT_NE(
            render(scene, kCanvas, open).rects,
            render(scene, kCanvas, blinking).rects);
    }

    TEST(PetSceneTest, TheIdleAnimationIsAFunctionOfTheTickCount)
    {
        const PetScene scene;

        PetSnapshot early = awake();
        PetSnapshot mid = awake();
        mid.ticks = antwika::companion::kTicksPerSecond / 2;

        const Drawn one = render(scene, kCanvas, early);
        const Drawn two = render(scene, kCanvas, mid);
        const Drawn again = render(scene, kCanvas, early);

        EXPECT_NE(one.rects, two.rects);
        EXPECT_EQ(one.rects, again.rects);
    }

    // Where the animal's body sits this frame.
    // Every pose of a breath lifts the whole animal together.
    [[nodiscard]] std::int32_t bodyTop(
        const PetScene &scene, const Tick tick)
    {
        PetSnapshot at = awake();
        at.ticks = tick;

        const auto rects = render(scene, kCanvas, at).rects;
        const auto wide = 12 * (kCanvas.width / kSceneUnits);

        // Named by shape rather than by index.
        // What is drawn before it depends on the state's own gauges.
        std::int32_t top = 0;

        for (const auto &rect : rects)
        {
            if (rect.size.width == wide)
            {
                top = rect.origin.y;
            }
        }

        return top;
    }

    // A breath is four poses over two seconds.
    constexpr Tick kBreathTicks = 2 * antwika::companion::kTicksPerSecond;

    // The bob is tweened rather than stepped between its four poses.
    // A whole unit is eight pixels at this canvas, over half a second.
    // Stepping it was a jolt; this is what says it no longer is.
    TEST(PetSceneTest, ABreathMovesTheAnimalByMoreThanItsTwoPoses)
    {
        const PetScene scene;

        std::set<std::int32_t> heights;

        for (Tick tick = 0; tick < kBreathTicks; ++tick)
        {
            heights.insert(bodyTop(scene, tick));
        }

        // Two is exactly what the stepped bob used to draw.
        EXPECT_GT(heights.size(), 2U);
    }

    // Easing must not move where a breath begins or how far it reaches.
    // Those two are what the stepped bob already drew.
    TEST(PetSceneTest, ABreathStillTravelsExactlyOneUnit)
    {
        const PetScene scene;

        std::set<std::int32_t> heights;

        for (Tick tick = 0; tick < kBreathTicks; ++tick)
        {
            heights.insert(bodyTop(scene, tick));
        }

        const auto unit = kCanvas.width / kSceneUnits;

        EXPECT_EQ(
            *heights.rbegin() - *heights.begin(),
            static_cast<std::int32_t>(unit));
        EXPECT_EQ(bodyTop(scene, 0), *heights.rbegin());
    }

    TEST(PetSceneTest, APerishedCompanionGetsAGraveAndItsOwnPalette)
    {
        const PetScene scene;

        const Drawn gone = render(scene, kCanvas, perished());
        const Drawn night = render(scene, kCanvas, asleep());

        EXPECT_NE(gone.cleared, night.cleared);
        EXPECT_NE(gone.cleared, render(scene, kCanvas, awake()).cleared);
    }

    TEST(PetSceneTest, Draw_OffersANewCompanionOnceItHasPerished)
    {
        const PetScene scene;
        const auto button = reviveButtonRect(kCanvas);
        ASSERT_TRUE(button.has_value());

        const Drawn drawn = render(scene, kCanvas, perished());

        EXPECT_TRUE(drew(drawn, *button));
        ASSERT_EQ(drawn.texts.size(), kReadoutLines + 1);
        EXPECT_EQ(drawn.texts.front().text, "new pet");
    }

    TEST(PetSceneTest, Draw_OffersNoButtonWhileTheCompanionIsAlive)
    {
        const PetScene scene;
        const auto button = reviveButtonRect(kCanvas);
        ASSERT_TRUE(button.has_value());

        const Drawn drawn = render(scene, kCanvas, awake());

        EXPECT_FALSE(drew(drawn, *button));
        EXPECT_EQ(drawn.texts.size(), kReadoutLines);
    }

    TEST(PetSceneTest, AnEmptyGaugeDrawsOnlyItsBackground)
    {
        const PetScene scene;

        PetSnapshot empty = awake();
        empty.fun = 0;
        empty.happiness = 0;

        const Drawn drawn = render(scene, kCanvas, empty);

        // Two fills fewer than the bare picture's.
        EXPECT_EQ(drawn.rects.size(), kBareAwakeRects - 2);
    }

    TEST(PetSceneTest, AGaugeWithNoMaximumDrawsOnlyItsBackground)
    {
        const PetScene scene;

        PetSnapshot broken = awake();
        broken.funMax = 0;
        broken.fun = 4;

        const Drawn drawn = render(scene, kCanvas, broken);

        EXPECT_EQ(drawn.rects.size(), kBareAwakeRects - 1);
    }

    TEST(PetSceneTest, AGaugeNeverFillsPastItsOwnWidth)
    {
        const PetScene scene;
        const auto layout = layoutFor(kCanvas);
        ASSERT_TRUE(layout.has_value());

        PetSnapshot overfull = awake();
        overfull.fun = overfull.funMax * 4;

        const Drawn drawn = render(scene, kCanvas, overfull);

        for (const Rect &rect : drawn.rects)
        {
            EXPECT_LE(
                rect.origin.x
                    + static_cast<std::int32_t>(rect.size.width),
                static_cast<std::int32_t>(kCanvas.width));
        }
    }

    // The life meter's own end moves as the companion collapses.
    // Its track is drawn to the ceiling it still has.
    // So a collapse shortens the bar and not merely its contents.
    TEST(PetSceneTest, TheEnergyGaugeFillsAgainstTheCeilingItHasLeft)
    {
        const PetScene scene;

        PetSnapshot full = awake();
        full.energy = 15;
        full.energyCeiling = 15;

        PetSnapshot half = awake();
        half.energy = 15;
        half.energyCeiling = 30;

        EXPECT_NE(
            render(scene, kCanvas, full).rects,
            render(scene, kCanvas, half).rects);
    }

    TEST(PetSceneTest, Draw_ReportsTheStateTheDayAndTheRecord)
    {
        const PetScene scene;

        PetSnapshot snapshot = awake();
        snapshot.day = 3;
        snapshot.stage = LifeStage::Teen;
        snapshot.mood = DayMood::Heavy;
        snapshot.lineage = LineageMemory{.generation = 2, .bestTicks = 90};

        const Drawn drawn = render(scene, kCanvas, snapshot);

        ASSERT_EQ(drawn.texts.size(), kReadoutLines);
        EXPECT_EQ(stateLine(drawn), "awake");
        EXPECT_EQ(dayLine(drawn), "d3 teen heavy");
        EXPECT_EQ(lineageLine(drawn), "gen 2 best 90");
    }

    // An ordinary day contributes nothing rather than a word for it.
    // So half of all days read shorter than the rest.
    TEST(PetSceneTest, Draw_SaysNothingAboutAnOrdinaryDay)
    {
        const PetScene scene;

        PetSnapshot snapshot = awake();
        snapshot.day = 1;
        snapshot.stage = LifeStage::Egg;
        snapshot.mood = DayMood::Ordinary;

        EXPECT_EQ(dayLine(render(scene, kCanvas, snapshot)), "d1 egg ");
    }

    TEST(PetSceneTest, Draw_SaysWhichOfItsStatesTheCompanionIsIn)
    {
        const PetScene scene;

        PetSnapshot hungry = awake();
        hungry.hungry = true;

        PetSnapshot woken = awake();
        woken.disturbed = true;

        PetSnapshot dozing = asleep();
        PetSnapshot dozingWoken = asleep();
        dozingWoken.disturbed = true;

        EXPECT_EQ(stateLine(render(scene, kCanvas, awake())), "awake");
        EXPECT_EQ(
            stateLine(render(scene, kCanvas, hungry)), "awake, hungry");
        EXPECT_EQ(
            stateLine(render(scene, kCanvas, woken)), "awake, woken");
        EXPECT_EQ(stateLine(render(scene, kCanvas, dozing)), "asleep");

        // Asleep beats woken.
        // What it is doing comes before how the day began.
        EXPECT_EQ(
            stateLine(render(scene, kCanvas, dozingWoken)), "asleep");
    }

    TEST(PetSceneTest, Draw_ReportsAPerishedCompanionToo)
    {
        const PetScene scene;

        EXPECT_EQ(stateLine(render(scene, kCanvas, perished())), "gone");
    }

    TEST(PetSceneTest, Draw_ScalesTheReadoutWithTheWindow)
    {
        const PetScene scene;

        const Drawn small = render(scene, kCanvas, awake());
        const Drawn large = render(
            scene, Size{.width = 512, .height = 512}, awake());

        ASSERT_FALSE(small.texts.empty());
        ASSERT_FALSE(large.texts.empty());
        EXPECT_GT(large.texts.front().scale, small.texts.front().scale);
    }

    TEST(PetSceneTest, Draw_KeepsTheSmallestReadoutOnTheGrid)
    {
        const PetScene scene;

        // A unit of two pixels is too small for four glyph pixels.
        const Drawn drawn =
            render(scene, Size{.width = 64, .height = 64}, awake());

        ASSERT_FALSE(drawn.texts.empty());
        EXPECT_EQ(drawn.texts.front().scale, 1U);
    }

    TEST(PetSceneTest, Draw_StacksTheReadoutOneLineHeightApart)
    {
        const PetScene scene;

        const Drawn drawn = render(scene, kCanvas, awake());

        ASSERT_EQ(drawn.texts.size(), kReadoutLines);
        const auto step = static_cast<std::int32_t>(
            kGlyphLineHeight * drawn.texts.front().scale);

        EXPECT_EQ(
            drawn.texts[1].origin.y, drawn.texts[0].origin.y + step);
        EXPECT_EQ(
            drawn.texts[2].origin.y, drawn.texts[0].origin.y + 2 * step);
    }

    TEST(PetSceneTest, Draw_DrawsNoBubbleWhileThereIsNothingToSay)
    {
        const PetScene scene;

        const Drawn drawn = render(scene, kCanvas, awake());

        EXPECT_EQ(drawn.rects.size(), kBareAwakeRects);
        EXPECT_EQ(drawn.texts.size(), kReadoutLines);
    }

    TEST(PetSceneTest, Draw_PutsWhatItSaysInABubbleBesideTheAnimal)
    {
        const PetScene scene;

        PetSnapshot talking = awake();
        talking.saying = Saying::Hello;

        const Drawn drawn = render(scene, kCanvas, talking);

        // The bubble and its tail, plus one more line of text.
        EXPECT_EQ(drawn.rects.size(), kBareAwakeRects + 2);
        ASSERT_EQ(drawn.texts.size(), kReadoutLines + 1);
        EXPECT_EQ(drawn.texts.front().text, "hello!");
    }

    TEST(PetSceneTest, Draw_SaysADifferentThingForADifferentLine)
    {
        const PetScene scene;
        std::vector<std::string> said;

        for (const Saying line : kEveryLine)
        {
            PetSnapshot talking = awake();
            talking.saying = line;

            const Drawn drawn = render(scene, kCanvas, talking);
            ASSERT_FALSE(drawn.texts.empty());
            said.push_back(drawn.texts.front().text);
        }

        std::vector<std::string> sorted = said;
        std::sort(sorted.begin(), sorted.end());
        EXPECT_EQ(
            std::unique(sorted.begin(), sorted.end()) - sorted.begin(),
            static_cast<std::ptrdiff_t>(kEveryLine.size()));

        for (const std::string &words : said)
        {
            EXPECT_FALSE(words.empty());
        }
    }

    TEST(PetSceneTest, Draw_KeepsEveryLineInsideItsOwnBubble)
    {
        const PetScene scene;

        for (const Saying line : kEveryLine)
        {
            PetSnapshot talking = awake();
            talking.saying = line;

            const Drawn drawn = render(scene, kCanvas, talking);
            const Rect bubble = bubbleOf(drawn);
            const Text &text = drawn.texts.front();
            const auto size =
                antwika::gfx::textSize(text.text, text.scale);

            EXPECT_GE(text.origin.x, bubble.origin.x);
            EXPECT_GE(text.origin.y, bubble.origin.y);
            EXPECT_LE(
                text.origin.x + static_cast<std::int32_t>(size.width),
                bubble.origin.x
                    + static_cast<std::int32_t>(bubble.size.width));
            EXPECT_LE(
                text.origin.y + static_cast<std::int32_t>(size.height),
                bubble.origin.y
                    + static_cast<std::int32_t>(bubble.size.height));
        }
    }

    // Under the gauges, so it covers nothing that says anything.
    TEST(PetSceneTest, Draw_KeepsTheBubbleClearOfTheGauges)
    {
        const PetScene scene;
        const auto layout = layoutFor(kCanvas);
        ASSERT_TRUE(layout.has_value());

        PetSnapshot talking = awake();
        talking.saying = Saying::Yum;

        const Drawn drawn = render(scene, kCanvas, talking);
        const Rect bubble = bubbleOf(drawn);

        // The four gauges take the top eight rows between them.
        EXPECT_GE(
            bubble.origin.y,
            8 * static_cast<std::int32_t>(layout->unit));
    }

    TEST(PetSceneTest, Draw_ScalesTheBubbleTextWithTheWindow)
    {
        const PetScene scene;

        PetSnapshot talking = awake();
        talking.saying = Saying::Yum;

        const Drawn small = render(scene, kCanvas, talking);
        const Drawn large = render(
            scene, Size{.width = 1024, .height = 1024}, talking);

        EXPECT_GT(large.texts.front().scale, small.texts.front().scale);
    }

    // A unit too small for even the smallest glyphs still gets them.
    TEST(PetSceneTest, Draw_KeepsTheSmallestBubbleTextReadable)
    {
        const PetScene scene;

        PetSnapshot talking = awake();
        talking.saying = Saying::Yum;

        const Drawn drawn =
            render(scene, Size{.width = 64, .height = 64}, talking);

        EXPECT_EQ(drawn.texts.front().scale, 1U);
    }

    TEST(PetSceneTest, Draw_GivesEveryUnitAWholeNumberOfPixels)
    {
        const PetScene scene;
        const auto layout = layoutFor(kCanvas);
        ASSERT_TRUE(layout.has_value());

        const Drawn drawn = render(scene, kCanvas, awake());

        for (const Rect &rect : drawn.rects)
        {
            EXPECT_EQ(rect.size.width % layout->unit, 0U);
            EXPECT_EQ(rect.size.height % layout->unit, 0U);
        }
    }
} // namespace
