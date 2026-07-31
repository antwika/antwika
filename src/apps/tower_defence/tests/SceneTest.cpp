#include <cstdint>
#include <variant>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/ui/DrawCommand.hpp>

#include "antwika/tower_defence/Battle.hpp"
#include "antwika/tower_defence/BattleScene.hpp"
#include "antwika/tower_defence/BattleSnapshot.hpp"
#include "antwika/tower_defence/GridLayout.hpp"
#include "antwika/tower_defence/Level.hpp"
#include "antwika/tower_defence/LevelTile.hpp"
#include "antwika/tower_defence/ScoreOverlay.hpp"
#include "antwika/tower_defence/ScoreSink.hpp"

using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::Size;
using antwika::tower_defence::Battle;
using antwika::tower_defence::BattleConfig;
using antwika::tower_defence::BattleScene;
using antwika::tower_defence::BattleSnapshot;
using antwika::tower_defence::Cell;
using antwika::tower_defence::cellAt;
using antwika::tower_defence::describeScoreBar;
using antwika::tower_defence::layoutFor;
using antwika::tower_defence::Level;
using antwika::tower_defence::rangeRadius;
using antwika::tower_defence::scoreBarHeight;
using antwika::tower_defence::ScoreOverlay;
using antwika::tower_defence::snapshotOf;
using antwika::tower_defence::Tile;
using ::testing::NiceMock;

namespace
{
    constexpr Size kCanvas{.width = 960, .height = 720};

    Level straightLevel(const std::uint32_t width)
    {
        Level level{
            .width = width,
            .height = 3,
            .tiles = std::vector<Tile>(
                static_cast<std::size_t>(width) * 3, Tile::Empty),
            .path = {}};
        for (std::uint32_t x = 0; x < width; ++x)
        {
            level.path.push_back({.x = x, .y = 0});
            level.tiles[x] = Tile::EastWest;
        }
        level.tiles[0] = Tile::Start;
        level.tiles[width - 1] = Tile::End;
        return level;
    }

    TEST(RangeRadiusTest, ASquaredRangeBecomesAWholeCellRadius)
    {
        EXPECT_EQ(rangeRadius(0), 0U);
        EXPECT_EQ(rangeRadius(1), 1U);
        EXPECT_EQ(rangeRadius(3), 1U);
        EXPECT_EQ(rangeRadius(4), 2U);
        EXPECT_EQ(rangeRadius(9), 3U);
    }

    TEST(BattleSnapshotTest, ASnapshotIsWhereEverythingStands)
    {
        Battle battle(
            straightLevel(6),
            BattleConfig{.spawnPeriodTicks = 1, .towerRangeSquared = 4});
        ASSERT_TRUE(battle.placeTower({.x = 2, .y = 1}));
        battle.step();
        battle.step();

        const BattleSnapshot snapshot = snapshotOf(battle);
        EXPECT_EQ(snapshot.level.width, 6U);
        EXPECT_EQ(snapshot.towers.size(), 1U);
        EXPECT_EQ(snapshot.towers[0], (Cell{.x = 2, .y = 1}));
        EXPECT_EQ(snapshot.mobs.size(), battle.mobs().size());
        EXPECT_EQ(snapshot.towerRangeSquared, 4U);
        EXPECT_EQ(snapshot.score, battle.score());
        EXPECT_EQ(snapshot.leaks, battle.leaks());
    }

    TEST(BattleSceneTest, EveryCellTowerHaloAndMobIsDrawn)
    {
        Battle battle(
            straightLevel(6),
            BattleConfig{.spawnPeriodTicks = 1000, .mobHealth = 99});
        ASSERT_TRUE(battle.placeTower({.x = 2, .y = 1}));
        battle.step();

        NiceMock<MockRenderer> renderer;
        EXPECT_CALL(renderer, clear(::testing::_)).Times(1);

        // Eighteen cells, one halo, one tower, one mob.
        EXPECT_CALL(renderer, drawRect(::testing::_, ::testing::_))
            .Times(21);

        const BattleScene scene;
        scene.draw(renderer, kCanvas, snapshotOf(battle));
    }

    TEST(BattleSceneTest, ACanvasWithNoRoomDrawsNothingButTheBackground)
    {
        Battle battle(straightLevel(6), BattleConfig{});
        NiceMock<MockRenderer> renderer;
        EXPECT_CALL(renderer, clear(::testing::_)).Times(1);
        EXPECT_CALL(renderer, drawRect(::testing::_, ::testing::_))
            .Times(0);

        const BattleScene scene;
        scene.draw(
            renderer, {.width = 4, .height = 4}, snapshotOf(battle));
    }

    TEST(BattleSceneTest, TheSameSnapshotAlwaysDrawsTheSameThing)
    {
        Battle battle(straightLevel(6), BattleConfig{});
        ASSERT_TRUE(battle.placeTower({.x = 1, .y = 2}));
        battle.step();

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
        scene.draw(first, kCanvas, snapshotOf(battle));
        scene.draw(second, kCanvas, snapshotOf(battle));
        EXPECT_EQ(firstRects, secondRects);
        EXPECT_FALSE(firstRects.empty());
    }

    TEST(ScoreOverlayTest, ItHoldsWhatWasPutInIt)
    {
        ScoreOverlay overlay(kCanvas);
        EXPECT_EQ(overlay.canvas(), kCanvas);
        EXPECT_TRUE(overlay.commands().empty());

        overlay.set(describeScoreBar(kCanvas, 40, 1));
        EXPECT_FALSE(overlay.commands().empty());
    }

    TEST(ScoreBarTest, ADifferentScoreIsADifferentPicture)
    {
        const auto quiet = describeScoreBar(kCanvas, 0, 0);
        const auto busy = describeScoreBar(kCanvas, 120, 3);
        EXPECT_FALSE(quiet.empty());
        EXPECT_NE(quiet, busy);

        // The bar is a pure function of the state it is given.
        EXPECT_EQ(busy, describeScoreBar(kCanvas, 120, 3));
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

        for (const Size canvas : canvases)
        {
            const auto commands = describeScoreBar(canvas, 1234, 5);
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

        const auto commands = describeScoreBar(canvas, 0, 0);
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
        Battle battle(straightLevel(6), BattleConfig{});
        ASSERT_TRUE(battle.placeTower({.x = 1, .y = 2}));

        // Six columns and three rows across six pixels of width.
        constexpr Size canvas{.width = 6, .height = 20};
        const auto layout = layoutFor(canvas, 6, 3);
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
        scene.draw(renderer, canvas, snapshotOf(battle));

        ASSERT_FALSE(rects.empty());
        for (const auto &rect : rects)
        {
            EXPECT_LE(rect.size.width, canvas.width);
            EXPECT_LE(rect.size.height, canvas.height);
        }
    }
} // namespace
