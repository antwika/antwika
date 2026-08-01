#include <cstddef>
#include <cstdint>
#include <variant>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/i18n/Translator.hpp>
#include <antwika/ui/DrawCommand.hpp>

#include "antwika/tower_defence/BattleScene.hpp"
#include "antwika/tower_defence/BattleSnapshot.hpp"
#include "antwika/tower_defence/Campaign.hpp"
#include "antwika/tower_defence/GridLayout.hpp"
#include "antwika/tower_defence/Level.hpp"
#include "antwika/tower_defence/LevelTile.hpp"
#include "antwika/tower_defence/MobKind.hpp"
#include "antwika/tower_defence/ScoreOverlay.hpp"
#include "antwika/tower_defence/ScoreSink.hpp"

using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::Size;
using antwika::i18n::Translator;
using antwika::tower_defence::BattleConfig;
using antwika::tower_defence::BattleScene;
using antwika::tower_defence::BattleSnapshot;
using antwika::tower_defence::Campaign;
using antwika::tower_defence::CampaignConfig;
using antwika::tower_defence::CampaignPhase;
using antwika::tower_defence::Cell;
using antwika::tower_defence::cellAt;
using antwika::tower_defence::describeScoreBar;
using antwika::tower_defence::layoutFor;
using antwika::tower_defence::Level;
using antwika::tower_defence::LevelPlan;
using antwika::tower_defence::MobKind;
using antwika::tower_defence::MobMarker;
using antwika::tower_defence::rangeRadius;
using antwika::tower_defence::scoreBarHeight;
using antwika::tower_defence::ScoreBarState;
using antwika::tower_defence::scoreBarStateOf;
using antwika::tower_defence::ScoreOverlay;
using antwika::tower_defence::snapshotOf;
using antwika::tower_defence::Tile;
using antwika::tower_defence::Wave;
using antwika::tower_defence::WaveEntry;
using ::testing::NiceMock;

namespace
{
    constexpr Size kCanvas{.width = 960, .height = 720};
    constexpr std::uint32_t kWidth = 6;
    constexpr std::uint32_t kHeight = 3;

    Translator english()
    {
        return Translator{antwika::i18n::kDefaultLocale};
    }

    // The smallest campaign that still generates a level.
    // So the solver is not what a drawing test costs.
    CampaignConfig tinyCampaign(const std::uint32_t mobs = 1)
    {
        return CampaignConfig{
            .seed = 2,
            .lives = 20,
            .levels = {LevelPlan{
                .level =
                    {.width = kWidth,
                     .height = kHeight,
                     .wallSpacing = 3},
                .battle =
                    BattleConfig{
                        .towerRangeSquared = 4, .towerDamage = 1},
                .waves = {Wave{
                    .entries = {WaveEntry{MobKind::Grunt, mobs}},
                    .spawnPeriodTicks = 1000,
                    .gapTicks = 0}}}}};
    }

    Cell anEmptyCell(const Campaign &campaign)
    {
        const auto &level = campaign.battle().level();
        for (std::uint32_t y = 0; y < level.height; ++y)
        {
            for (std::uint32_t x = 0; x < level.width; ++x)
            {
                const Cell cell{.x = x, .y = y};
                if (level.at(cell) == Tile::Empty)
                {
                    return cell;
                }
            }
        }
        return Cell{};
    }

    TEST(RangeRadiusTest, ASquaredRangeBecomesAWholeCellRadius)
    {
        EXPECT_EQ(rangeRadius(0), 0U);
        EXPECT_EQ(rangeRadius(1), 1U);
        EXPECT_EQ(rangeRadius(3), 1U);
        EXPECT_EQ(rangeRadius(4), 2U);
        EXPECT_EQ(rangeRadius(9), 3U);
    }

    TEST(BattleSnapshotTest, ASnapshotIsWhereEverythingStandsAndWhatItIs)
    {
        Campaign campaign(tinyCampaign());
        const Cell open = anEmptyCell(campaign);
        ASSERT_TRUE(campaign.placeTower(open));
        campaign.step();

        const BattleSnapshot snapshot = snapshotOf(campaign);
        EXPECT_EQ(snapshot.level.width, kWidth);
        ASSERT_EQ(snapshot.towers.size(), 1U);
        EXPECT_EQ(snapshot.towers[0], open);
        EXPECT_EQ(snapshot.towerRangeSquared, 4U);

        ASSERT_EQ(snapshot.mobs.size(), campaign.battle().mobs().size());
        ASSERT_FALSE(snapshot.mobs.empty());
        EXPECT_EQ(snapshot.mobs[0].kind, MobKind::Grunt);
        EXPECT_EQ(
            snapshot.mobs[0].cell, campaign.battle().level().path[0]);
    }

    TEST(BattleSceneTest, EveryCellTowerHaloAndMobIsDrawn)
    {
        Campaign campaign(tinyCampaign());
        ASSERT_TRUE(campaign.placeTower(anEmptyCell(campaign)));
        campaign.step();
        ASSERT_EQ(campaign.battle().mobs().size(), 1U);

        NiceMock<MockRenderer> renderer;
        EXPECT_CALL(renderer, clear(::testing::_)).Times(1);

        // Eighteen cells, one halo, one tower, one mob.
        EXPECT_CALL(renderer, drawRect(::testing::_, ::testing::_))
            .Times(21);

        const BattleScene scene;
        scene.draw(renderer, kCanvas, snapshotOf(campaign));
    }

    TEST(BattleSceneTest, ACanvasWithNoRoomDrawsNothingButTheBackground)
    {
        const Campaign campaign(tinyCampaign());
        NiceMock<MockRenderer> renderer;
        EXPECT_CALL(renderer, clear(::testing::_)).Times(1);
        EXPECT_CALL(renderer, drawRect(::testing::_, ::testing::_))
            .Times(0);

        const BattleScene scene;
        scene.draw(
            renderer, {.width = 4, .height = 4}, snapshotOf(campaign));
    }

    TEST(BattleSceneTest, TheSameSnapshotAlwaysDrawsTheSameThing)
    {
        Campaign campaign(tinyCampaign());
        ASSERT_TRUE(campaign.placeTower(anEmptyCell(campaign)));
        campaign.step();

        NiceMock<MockRenderer> first;
        NiceMock<MockRenderer> second;
        std::vector<antwika::gfx::Rect> firstRects;
        std::vector<antwika::gfx::Rect> secondRects;
        ON_CALL(first, drawRect)
            .WillByDefault(
                [&firstRects](
                    const antwika::gfx::Rect rect, antwika::gfx::Color)
                { firstRects.push_back(rect); });
        ON_CALL(second, drawRect)
            .WillByDefault(
                [&secondRects](
                    const antwika::gfx::Rect rect, antwika::gfx::Color)
                { secondRects.push_back(rect); });

        const BattleScene scene;
        scene.draw(first, kCanvas, snapshotOf(campaign));
        scene.draw(second, kCanvas, snapshotOf(campaign));
        EXPECT_EQ(firstRects, secondRects);
        EXPECT_FALSE(firstRects.empty());
    }

    // Four kinds, four colours.
    // So which of them is on the road is read off the grid.
    TEST(BattleSceneTest, EachKindIsDrawnInItsOwnColour)
    {
        const Level ground{
            .width = 1,
            .height = 1,
            .tiles = {Tile::Start},
            .path = {Cell{.x = 0, .y = 0}}};

        std::vector<antwika::gfx::Color> inks;
        for (const MobKind kind : antwika::tower_defence::kAllMobKinds)
        {
            const BattleSnapshot snapshot{
                .level = ground,
                .mobs = {MobMarker{.cell = {.x = 0, .y = 0},
                                   .kind = kind}},
                .towers = {},
                .towerRangeSquared = 0};

            NiceMock<MockRenderer> renderer;
            std::vector<antwika::gfx::Color> drawn;
            ON_CALL(renderer, drawRect)
                .WillByDefault(
                    [&drawn](
                        antwika::gfx::Rect, const antwika::gfx::Color ink)
                    { drawn.push_back(ink); });

            const BattleScene scene;
            scene.draw(renderer, kCanvas, snapshot);

            ASSERT_FALSE(drawn.empty());
            inks.push_back(drawn.back());
        }

        for (std::size_t i = 1; i < inks.size(); ++i)
        {
            for (std::size_t j = 0; j < i; ++j)
            {
                EXPECT_NE(inks[i], inks[j]) << "kinds " << j << " " << i;
            }
        }
    }

    TEST(ScoreOverlayTest, ItHoldsWhatWasPutInIt)
    {
        ScoreOverlay overlay(kCanvas);
        EXPECT_EQ(overlay.canvas(), kCanvas);
        EXPECT_TRUE(overlay.commands().empty());

        overlay.set(describeScoreBar(
            kCanvas, english(), ScoreBarState{.score = 40}));
        EXPECT_FALSE(overlay.commands().empty());
    }

    TEST(ScoreBarTest, ADifferentStateIsADifferentPicture)
    {
        const Translator translator = english();
        const auto quiet =
            describeScoreBar(kCanvas, translator, ScoreBarState{});
        const auto busy = describeScoreBar(
            kCanvas,
            translator,
            ScoreBarState{
                .level = 2,
                .levelCount = 3,
                .wave = 2,
                .waveCount = 4,
                .lives = 7,
                .score = 120,
                .best = 400});

        EXPECT_FALSE(quiet.empty());
        EXPECT_NE(quiet, busy);

        // The bar is a pure function of the state it is given.
        EXPECT_EQ(
            busy,
            describeScoreBar(
                kCanvas,
                translator,
                ScoreBarState{
                    .level = 2,
                    .levelCount = 3,
                    .wave = 2,
                    .waveCount = 4,
                    .lives = 7,
                    .score = 120,
                    .best = 400}));
    }

    // Once a campaign is over the bar says how it ended.
    // Rather than which wave is out, since there is no next wave.
    TEST(ScoreBarTest, TheOutcomeReplacesTheWaveCountOnceItIsOver)
    {
        const Translator translator = english();
        const ScoreBarState fighting{.phase = CampaignPhase::Fighting};
        const ScoreBarState won{.phase = CampaignPhase::Won};
        const ScoreBarState lost{.phase = CampaignPhase::Lost};

        const auto drawn = [&translator](const ScoreBarState &state)
        { return describeScoreBar(kCanvas, translator, state); };

        EXPECT_NE(drawn(fighting), drawn(won));
        EXPECT_NE(drawn(fighting), drawn(lost));
        EXPECT_NE(drawn(won), drawn(lost));
    }

    // A finished campaign steps its level index past the last level.
    // A level whose waves are all out does the same with its own.
    // The bar clamps both rather than counting past the total.
    TEST(ScoreBarStateTest, TheCountsAreClampedToTheirTotals)
    {
        Campaign campaign(tinyCampaign());

        const ScoreBarState opening = scoreBarStateOf(campaign, 900);
        EXPECT_EQ(opening.level, 1U);
        EXPECT_EQ(opening.levelCount, 1U);
        EXPECT_EQ(opening.wave, 1U);
        EXPECT_EQ(opening.waveCount, 1U);
        EXPECT_EQ(opening.best, 900U);
        EXPECT_EQ(opening.phase, CampaignPhase::Fighting);

        for (int step = 0; step < 200; ++step)
        {
            campaign.step();
        }
        ASSERT_EQ(campaign.phase(), CampaignPhase::Won);

        const ScoreBarState ended = scoreBarStateOf(campaign, 900);
        EXPECT_EQ(ended.level, 1U);
        EXPECT_EQ(ended.wave, 1U);
        EXPECT_EQ(ended.phase, CampaignPhase::Won);
    }

    // The whole reason no sink asks the UI whether it covered a click.
    // A strip shorter than the bar leaves the bar over the grid.
    // A press there would build a tower nobody could see.
    TEST(ScoreBarStripTest, TheStripIsExactlyAsTallAsTheBarDrawnInIt)
    {
        constexpr Size canvases[] = {
            {.width = 960, .height = 720},
            {.width = 800, .height = 600},
            {.width = 1920, .height = 1080},
            {.width = 400, .height = 240},
            {.width = 200, .height = 120}};

        const Translator translator = english();
        const ScoreBarState state{
            .level = 2,
            .levelCount = 3,
            .wave = 3,
            .waveCount = 4,
            .lives = 5,
            .score = 1234,
            .best = 4321};

        for (const Size canvas : canvases)
        {
            const auto commands =
                describeScoreBar(canvas, translator, state);
            ASSERT_FALSE(commands.empty());

            // The panel is the first thing the bar fills.
            const auto *panel =
                std::get_if<antwika::ui::FillRect>(&commands.front());
            ASSERT_NE(panel, nullptr);
            EXPECT_EQ(panel->rect.origin.y, 0);
            EXPECT_EQ(panel->rect.size.height, scoreBarHeight(canvas));
        }
    }

    // 1920x1080 used to reserve 48 pixels for a 64-pixel bar.
    // The top grid row sat under it and took presses meant for it.
    TEST(ScoreBarStripTest, NoGridRowEverStartsUnderTheBar)
    {
        constexpr Size canvas{.width = 1920, .height = 1080};

        const auto layout = layoutFor(canvas, 12, 8);
        ASSERT_TRUE(layout.has_value());
        EXPECT_GE(
            layout->origin.y,
            static_cast<std::int32_t>(scoreBarHeight(canvas)));

        const auto commands =
            describeScoreBar(canvas, english(), ScoreBarState{});
        const auto *panel =
            std::get_if<antwika::ui::FillRect>(&commands.front());
        ASSERT_NE(panel, nullptr);
        const auto barBottom = static_cast<std::int32_t>(
            panel->rect.size.height);
        EXPECT_FALSE(cellAt(*layout, 100, barBottom - 1).has_value());
    }

    // layoutFor() refuses a cell of zero pixels and no smaller size.
    // So one pixel is a cell the scene has to survive drawing.
    // An unsaturated inset turns a one-pixel width into four billion.
    TEST(BattleSceneTest, AOnePixelCellDrawsNothingWiderThanTheCell)
    {
        Campaign campaign(tinyCampaign());
        ASSERT_TRUE(campaign.placeTower(anEmptyCell(campaign)));

        // Six columns and three rows across six pixels of width.
        constexpr Size canvas{.width = 6, .height = 20};
        const auto layout = layoutFor(canvas, kWidth, kHeight);
        ASSERT_TRUE(layout.has_value());
        ASSERT_EQ(layout->cell, 1U);

        NiceMock<MockRenderer> renderer;
        std::vector<antwika::gfx::Rect> rects;
        ON_CALL(renderer, drawRect)
            .WillByDefault(
                [&rects](
                    const antwika::gfx::Rect rect, antwika::gfx::Color)
                { rects.push_back(rect); });

        const BattleScene scene;
        scene.draw(renderer, canvas, snapshotOf(campaign));

        ASSERT_FALSE(rects.empty());
        for (const auto &rect : rects)
        {
            EXPECT_LE(rect.size.width, canvas.width);
            EXPECT_LE(rect.size.height, canvas.height);
        }
    }
} // namespace
