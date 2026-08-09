#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Glyphs.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/TextLayout.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/companion/DayMood.hpp"
#include "antwika/companion/LifeStage.hpp"
#include "antwika/companion/Messages.hpp"
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
using antwika::companion::propArtBox;
using antwika::companion::propBox;
using antwika::companion::propLabelBox;
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
    constexpr Size kCanvas{.width = 256, .height = 256};

    constexpr std::size_t kBareAwakeRects = 25;

    constexpr std::size_t kReadoutLines = 3;

    constexpr std::size_t kPropLabels = 3;

    struct Text final
    {
        Point origin{};
        std::string text;
        std::uint32_t scale = 0;
        Color color{};
    };

    struct Drawn final
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

    [[nodiscard]] const Text &readoutText(const Drawn &drawn)
    {
        return drawn.texts[drawn.texts.size() - kReadoutLines];
    }

    [[nodiscard]] const Text &bubbleText(const Drawn &drawn)
    {
        return drawn.texts[drawn.texts.size() - kReadoutLines - 1];
    }

    [[nodiscard]] const Text &labelText(
        const Drawn &drawn, const Prop prop)
    {
        return drawn.texts[static_cast<std::size_t>(prop)];
    }

    [[nodiscard]] bool within(const Rect &area, const Text &text)
    {
        const auto size = antwika::gfx::textSize(text.text, text.scale);

        return text.origin.x >= area.origin.x
               && text.origin.y >= area.origin.y
               && text.origin.x
                      + static_cast<std::int32_t>(size.width)
                  <= area.origin.x
                         + static_cast<std::int32_t>(area.size.width)
               && text.origin.y
                      + static_cast<std::int32_t>(size.height)
                  <= area.origin.y
                         + static_cast<std::int32_t>(area.size.height);
    }

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

    TEST(PetSceneTest, Render_DrawsOnlyTheSkyOnATinyCanvas)
    {
        const antwika::companion::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

        const Drawn drawn =
            render(scene, Size{.width = 16, .height = 16}, awake());

        EXPECT_TRUE(drawn.rects.empty());
        EXPECT_TRUE(drawn.texts.empty());
    }

    TEST(PetSceneTest, Render_CentresTheSquareOnTheLongerSide)
    {
        const antwika::companion::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

        const Drawn drawn =
            render(scene, Size{.width = 320, .height = 256}, awake());

        ASSERT_FALSE(drawn.rects.empty());
        EXPECT_EQ(drawn.rects[0].origin.x, 32);
        EXPECT_EQ(drawn.rects[0].origin.y, 176);
    }

    TEST(PetSceneTest, Render_DrawsTheBarePictureWhenUnhungry)
    {
        const antwika::companion::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

        const Drawn drawn = render(scene, kCanvas, awake());

        EXPECT_EQ(drawn.rects.size(), kBareAwakeRects);
        EXPECT_EQ(drawn.texts.size(), kReadoutLines + kPropLabels);
    }

    TEST(PetSceneTest, Render_DrawsNightDifferentlyFromDay)
    {
        const antwika::companion::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

        const Drawn day = render(scene, kCanvas, awake());
        const Drawn night = render(scene, kCanvas, asleep());

        EXPECT_NE(day.cleared, night.cleared);
    }

    TEST(PetSceneTest, Render_PaintsEveryPropIntoItsHitBox)
    {
        const antwika::companion::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};
        const auto layout = layoutFor(kCanvas);
        ASSERT_TRUE(layout.has_value());

        const Drawn drawn = render(scene, kCanvas, awake());

        for (const Prop prop : kProps)
        {
            EXPECT_TRUE(drew(drawn, propBox(*layout, prop)));
        }
    }

    TEST(PetSceneTest, Render_LitsOnlyTheWantedProp)
    {
        const antwika::companion::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};
        const auto layout = layoutFor(kCanvas);
        ASSERT_TRUE(layout.has_value());

        const auto innerOf = [&layout](const Drawn &drawn, const Prop p)
        {
            const Rect area = propArtBox(*layout, p);
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

    TEST(PetSceneTest, Draw_NamesEveryPropInTheBoxThatPressesIt)
    {
        const antwika::companion::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};
        const auto layout = layoutFor(kCanvas);
        ASSERT_TRUE(layout.has_value());

        const std::array<std::string, 3> named{"feed", "play", "sleep"};
        const Drawn drawn = render(scene, kCanvas, awake());

        ASSERT_EQ(drawn.texts.size(), kReadoutLines + kPropLabels);

        for (const Prop prop : kProps)
        {
            const Text &label = labelText(drawn, prop);

            EXPECT_EQ(
                label.text, named[static_cast<std::size_t>(prop)]);
            EXPECT_TRUE(within(propLabelBox(*layout, prop), label));
        }
    }

    TEST(PetSceneTest, Draw_WordsThePropLabelsInTheTranslatorsLanguage)
    {
        const antwika::companion::Translator swedish{
            antwika::i18n::Locale::Swedish};
        const PetScene scene{swedish};

        const Drawn drawn = render(scene, kCanvas, awake());

        EXPECT_EQ(labelText(drawn, Prop::Bowl).text, "mata");
        EXPECT_EQ(labelText(drawn, Prop::Ball).text, "leka");
        EXPECT_EQ(labelText(drawn, Prop::Nest).text, "sova");
    }

    TEST(PetSceneTest, Draw_ScalesThePropLabelsWithTheWindow)
    {
        const antwika::companion::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

        const Drawn small = render(scene, kCanvas, awake());
        const Drawn large = render(
            scene, Size{.width = 512, .height = 512}, awake());

        EXPECT_GT(
            labelText(large, Prop::Bowl).scale,
            labelText(small, Prop::Bowl).scale);
    }

    TEST(PetSceneTest, Draw_KeepsTheSmallestPropLabelReadable)
    {
        const antwika::companion::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

        const Drawn drawn =
            render(scene, Size{.width = 64, .height = 64}, awake());

        EXPECT_EQ(labelText(drawn, Prop::Bowl).scale, 1U);
    }

    TEST(PetSceneTest, Draw_GivesTheThreeLabelsOneSizeBetweenThem)
    {
        const antwika::companion::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

        const Drawn drawn = render(scene, kCanvas, awake());

        EXPECT_EQ(
            labelText(drawn, Prop::Bowl).scale,
            labelText(drawn, Prop::Nest).scale);
        EXPECT_EQ(
            labelText(drawn, Prop::Ball).scale,
            labelText(drawn, Prop::Nest).scale);
    }

    TEST(PetSceneTest, Render_LightsThePropForEachNeed)
    {
        const antwika::companion::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

        PetSnapshot bored = awake();
        bored.bored = true;
        PetSnapshot tired = awake();
        tired.tired = true;

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

    TEST(PetSceneTest, Render_ShadesTheFurByTheGrownForm)
    {
        const antwika::companion::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

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

    TEST(PetSceneTest, Render_ShutsTheEyesAndPuffsWhenAsleep)
    {
        const antwika::companion::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

        const Drawn up = render(scene, kCanvas, awake());
        const Drawn down = render(scene, kCanvas, asleep());

        EXPECT_GT(down.rects.size(), up.rects.size());
    }

    TEST(PetSceneTest, Render_AddsAPuffPerDrowseFrame)
    {
        const antwika::companion::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

        PetSnapshot first = asleep();
        PetSnapshot later = asleep();
        later.ticks = 3 * antwika::companion::kTicksPerSecond / 4;

        const Drawn one = render(scene, kCanvas, first);
        const Drawn two = render(scene, kCanvas, later);

        EXPECT_EQ(two.rects.size(), one.rects.size() + 1);
    }

    TEST(PetSceneTest, Render_BlinksAnAwakeCompanion)
    {
        const antwika::companion::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

        PetSnapshot open = awake();
        PetSnapshot blinking = awake();
        blinking.ticks = 3 * antwika::companion::kTicksPerSecond + 1;

        EXPECT_NE(
            render(scene, kCanvas, open).rects,
            render(scene, kCanvas, blinking).rects);
    }

    TEST(PetSceneTest, Render_DrivesTheIdleClipFromTheTick)
    {
        const antwika::companion::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

        PetSnapshot early = awake();
        PetSnapshot mid = awake();
        mid.ticks = antwika::companion::kTicksPerSecond / 2;

        const Drawn one = render(scene, kCanvas, early);
        const Drawn two = render(scene, kCanvas, mid);
        const Drawn again = render(scene, kCanvas, early);

        EXPECT_NE(one.rects, two.rects);
        EXPECT_EQ(one.rects, again.rects);
    }

    [[nodiscard]] std::int32_t bodyTop(
        const PetScene &scene, const Tick tick)
    {
        PetSnapshot at = awake();
        at.ticks = tick;

        const auto rects = render(scene, kCanvas, at).rects;
        const auto wide = 12 * (kCanvas.width / kSceneUnits);

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

    constexpr Tick kBreathTicks = 2 * antwika::companion::kTicksPerSecond;

    TEST(PetSceneTest, Render_MovesTheBreathBeyondTwoPoses)
    {
        const antwika::companion::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

        std::set<std::int32_t> heights;

        for (Tick tick = 0; tick < kBreathTicks; ++tick)
        {
            heights.insert(bodyTop(scene, tick));
        }

        EXPECT_GT(heights.size(), 2U);
    }

    TEST(PetSceneTest, Render_TravelsExactlyOneUnitPerBreath)
    {
        const antwika::companion::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

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

    TEST(PetSceneTest, Render_GivesAPerishedOneAGraveAndPalette)
    {
        const antwika::companion::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

        const Drawn gone = render(scene, kCanvas, perished());
        const Drawn night = render(scene, kCanvas, asleep());

        EXPECT_NE(gone.cleared, night.cleared);
        EXPECT_NE(gone.cleared, render(scene, kCanvas, awake()).cleared);
    }

    TEST(PetSceneTest, Draw_OffersANewCompanionOnceItHasPerished)
    {
        const antwika::companion::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};
        const auto button = reviveButtonRect(kCanvas);
        ASSERT_TRUE(button.has_value());

        const Drawn drawn = render(scene, kCanvas, perished());

        EXPECT_TRUE(drew(drawn, *button));
        ASSERT_EQ(drawn.texts.size(), kReadoutLines + 1);
        EXPECT_EQ(drawn.texts.front().text, "new pet");
    }

    TEST(PetSceneTest, Draw_OffersNoButtonWhileTheCompanionIsAlive)
    {
        const antwika::companion::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};
        const auto button = reviveButtonRect(kCanvas);
        ASSERT_TRUE(button.has_value());

        const Drawn drawn = render(scene, kCanvas, awake());

        EXPECT_FALSE(drew(drawn, *button));
        EXPECT_EQ(drawn.texts.size(), kReadoutLines + kPropLabels);
    }

    TEST(PetSceneTest, Render_DrawsOnlyTheBackgroundOfAnEmptyGauge)
    {
        const antwika::companion::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

        PetSnapshot empty = awake();
        empty.fun = 0;
        empty.happiness = 0;

        const Drawn drawn = render(scene, kCanvas, empty);

        EXPECT_EQ(drawn.rects.size(), kBareAwakeRects - 2);
    }

    TEST(PetSceneTest, Render_DrawsNoFillWithoutAMaximum)
    {
        const antwika::companion::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

        PetSnapshot broken = awake();
        broken.funMax = 0;
        broken.fun = 4;

        const Drawn drawn = render(scene, kCanvas, broken);

        EXPECT_EQ(drawn.rects.size(), kBareAwakeRects - 1);
    }

    TEST(PetSceneTest, Render_NeverFillsAGaugePastItsWidth)
    {
        const antwika::companion::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};
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

    TEST(PetSceneTest, Render_FillsEnergyAgainstItsCeiling)
    {
        const antwika::companion::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

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
        const antwika::companion::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

        PetSnapshot snapshot = awake();
        snapshot.day = 3;
        snapshot.stage = LifeStage::Teen;
        snapshot.mood = DayMood::Heavy;
        snapshot.lineage = LineageMemory{.generation = 2, .bestTicks = 90};

        const Drawn drawn = render(scene, kCanvas, snapshot);

        ASSERT_EQ(drawn.texts.size(), kReadoutLines + kPropLabels);
        EXPECT_EQ(stateLine(drawn), "awake");
        EXPECT_EQ(dayLine(drawn), "d3 teen heavy");
        EXPECT_EQ(lineageLine(drawn), "gen 2 best 90");
    }

    TEST(PetSceneTest, Draw_SaysNothingAboutAnOrdinaryDay)
    {
        const antwika::companion::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

        PetSnapshot snapshot = awake();
        snapshot.day = 1;
        snapshot.stage = LifeStage::Egg;
        snapshot.mood = DayMood::Ordinary;

        EXPECT_EQ(dayLine(render(scene, kCanvas, snapshot)), "d1 egg ");
    }

    TEST(PetSceneTest, Draw_SaysWhichOfItsStatesTheCompanionIsIn)
    {
        const antwika::companion::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

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

        EXPECT_EQ(
            stateLine(render(scene, kCanvas, dozingWoken)), "asleep");
    }

    TEST(PetSceneTest, Draw_ReportsAPerishedCompanionToo)
    {
        const antwika::companion::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

        EXPECT_EQ(stateLine(render(scene, kCanvas, perished())), "gone");
    }

    TEST(PetSceneTest, Draw_ScalesTheReadoutWithTheWindow)
    {
        const antwika::companion::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

        const Drawn small = render(scene, kCanvas, awake());
        const Drawn large = render(
            scene, Size{.width = 512, .height = 512}, awake());

        ASSERT_FALSE(small.texts.empty());
        ASSERT_FALSE(large.texts.empty());
        EXPECT_GT(readoutText(large).scale, readoutText(small).scale);
    }

    TEST(PetSceneTest, Draw_KeepsTheSmallestReadoutOnTheGrid)
    {
        const antwika::companion::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

        const Drawn drawn =
            render(scene, Size{.width = 64, .height = 64}, awake());

        ASSERT_FALSE(drawn.texts.empty());
        EXPECT_EQ(readoutText(drawn).scale, 1U);
    }

    TEST(PetSceneTest, Draw_StacksTheReadoutOneLineHeightApart)
    {
        const antwika::companion::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

        const Drawn drawn = render(scene, kCanvas, awake());

        ASSERT_EQ(drawn.texts.size(), kReadoutLines + kPropLabels);
        const auto step = static_cast<std::int32_t>(
            kGlyphLineHeight * readoutText(drawn).scale);
        const auto first = drawn.texts.size() - kReadoutLines;

        EXPECT_EQ(
            drawn.texts[first + 1].origin.y,
            drawn.texts[first].origin.y + step);
        EXPECT_EQ(
            drawn.texts[first + 2].origin.y,
            drawn.texts[first].origin.y + 2 * step);
    }

    TEST(PetSceneTest, Draw_DrawsNoBubbleWhileThereIsNothingToSay)
    {
        const antwika::companion::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

        PetSnapshot quiet = awake();
        quiet.saying = Saying::None;

        PetSnapshot talking = awake();
        talking.saying = Saying::Hello;

        const Drawn silent = render(scene, kCanvas, quiet);
        const Drawn spoken = render(scene, kCanvas, talking);

        EXPECT_EQ(silent.rects.size() + 2, spoken.rects.size());
        EXPECT_EQ(silent.texts.size() + 1, spoken.texts.size());
        EXPECT_NE(bubbleOf(silent), bubbleOf(spoken));
    }

    TEST(PetSceneTest, Draw_PutsWhatItSaysInABubbleBesideTheAnimal)
    {
        const antwika::companion::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

        PetSnapshot talking = awake();
        talking.saying = Saying::Hello;

        const Drawn drawn = render(scene, kCanvas, talking);

        EXPECT_EQ(drawn.rects.size(), kBareAwakeRects + 2);
        ASSERT_EQ(
            drawn.texts.size(), kReadoutLines + kPropLabels + 1);
        EXPECT_EQ(bubbleText(drawn).text, "hello!");
    }

    TEST(PetSceneTest, Draw_SaysADifferentThingForADifferentLine)
    {
        const antwika::companion::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};
        std::vector<std::string> said;

        for (const Saying line : kEveryLine)
        {
            PetSnapshot talking = awake();
            talking.saying = line;

            const Drawn drawn = render(scene, kCanvas, talking);
            ASSERT_FALSE(drawn.texts.empty());
            said.push_back(bubbleText(drawn).text);
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
        const antwika::companion::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

        for (const Saying line : kEveryLine)
        {
            PetSnapshot talking = awake();
            talking.saying = line;

            const Drawn drawn = render(scene, kCanvas, talking);
            EXPECT_TRUE(within(bubbleOf(drawn), bubbleText(drawn)));
        }
    }

    TEST(PetSceneTest, Draw_KeepsTheBubbleClearOfTheGauges)
    {
        const antwika::companion::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};
        const auto layout = layoutFor(kCanvas);
        ASSERT_TRUE(layout.has_value());

        PetSnapshot talking = awake();
        talking.saying = Saying::Yum;

        const Drawn drawn = render(scene, kCanvas, talking);
        const Rect bubble = bubbleOf(drawn);

        EXPECT_GE(
            bubble.origin.y,
            8 * static_cast<std::int32_t>(layout->unit));
    }

    TEST(PetSceneTest, Draw_ScalesTheBubbleTextWithTheWindow)
    {
        const antwika::companion::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

        PetSnapshot talking = awake();
        talking.saying = Saying::Yum;

        const Drawn small = render(scene, kCanvas, talking);
        const Drawn large = render(
            scene, Size{.width = 1024, .height = 1024}, talking);

        EXPECT_GT(bubbleText(large).scale, bubbleText(small).scale);
    }

    TEST(PetSceneTest, Draw_KeepsTheSmallestBubbleTextReadable)
    {
        const antwika::companion::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};

        PetSnapshot talking = awake();
        talking.saying = Saying::Yum;

        const Drawn drawn =
            render(scene, Size{.width = 64, .height = 64}, talking);

        EXPECT_EQ(bubbleText(drawn).scale, 1U);
    }

    TEST(PetSceneTest, Draw_GivesEveryUnitAWholeNumberOfPixels)
    {
        const antwika::companion::Translator translator{
            antwika::i18n::kDefaultLocale};
        const PetScene scene{translator};
        const auto layout = layoutFor(kCanvas);
        ASSERT_TRUE(layout.has_value());

        const Drawn drawn = render(scene, kCanvas, awake());

        for (const Rect &rect : drawn.rects)
        {
            EXPECT_EQ(rect.size.width % layout->unit, 0U);
            EXPECT_EQ(rect.size.height % layout->unit, 0U);
        }
    }

    TEST(PetSceneTest, Draw_SaysEveryWordInTheTranslatorsLanguage)
    {
        const antwika::companion::Translator swedish{
            antwika::i18n::Locale::Swedish};
        const PetScene scene{swedish};

        PetSnapshot talking = awake();
        talking.hungry = true;
        talking.saying = Saying::FeedMe;

        const Drawn drawn = render(scene, kCanvas, talking);

        ASSERT_EQ(
            drawn.texts.size(), kReadoutLines + kPropLabels + 1);
        EXPECT_EQ(bubbleText(drawn).text, "mata mig!");
        EXPECT_EQ(stateLine(drawn), "vaken, hungrig");
        EXPECT_EQ(dayLine(drawn), "d0 ägg ");
        EXPECT_EQ(lineageLine(drawn), "gen 1 bäst 0");
    }

    TEST(PetSceneTest, Draw_WordsTheNewCompanionButtonAsWell)
    {
        const antwika::companion::Translator swedish{
            antwika::i18n::Locale::Swedish};
        const PetScene scene{swedish};

        PetSnapshot gone = awake();
        gone.state = PetState::Perished;

        const Drawn drawn = render(scene, kCanvas, gone);

        EXPECT_EQ(drawn.texts[0].text, "nytt djur");
        EXPECT_EQ(stateLine(drawn), "borta");
    }

    TEST(PetSceneTest, Draw_ScalesTheBubbleToTheLongestLineInUse)
    {
        const antwika::companion::Translator english{
            antwika::i18n::Locale::English};
        const antwika::companion::Translator swedish{
            antwika::i18n::Locale::Swedish};

        PetSnapshot talking = awake();
        talking.saying = Saying::Hello;

        const Drawn drawn = render(
            PetScene{english}, {.width = 512, .height = 512}, talking);
        const Drawn other = render(
            PetScene{swedish}, {.width = 512, .height = 512}, talking);

        EXPECT_GT(bubbleText(drawn).scale, bubbleText(other).scale);
    }
}
