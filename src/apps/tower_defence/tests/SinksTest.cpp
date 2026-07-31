#include <chrono>
#include <cstdint>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockWindow.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/time/fakes/FakeSleeper.hpp>

#include "antwika/tower_defence/Battle.hpp"
#include "antwika/tower_defence/BattleScene.hpp"
#include "antwika/tower_defence/BattleSink.hpp"
#include "antwika/tower_defence/GridLayout.hpp"
#include "antwika/tower_defence/Level.hpp"
#include "antwika/tower_defence/LevelTile.hpp"
#include "antwika/tower_defence/RenderSink.hpp"
#include "antwika/tower_defence/ScoreOverlay.hpp"
#include "antwika/tower_defence/ScoreSink.hpp"
#include "antwika/tower_defence/TowerPlacementSink.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::mocks::MockWindow;
using antwika::gfx::Size;
using antwika::input::InputEventCodec;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::input::PointerButtonReleased;
using antwika::time::fakes::FakeSleeper;
using antwika::tower_defence::Battle;
using antwika::tower_defence::BattleConfig;
using antwika::tower_defence::BattleScene;
using antwika::tower_defence::BattleSink;
using antwika::tower_defence::Cell;
using antwika::tower_defence::cellRect;
using antwika::tower_defence::layoutFor;
using antwika::tower_defence::Level;
using antwika::tower_defence::RenderSink;
using antwika::tower_defence::ScoreOverlay;
using antwika::tower_defence::ScoreSink;
using antwika::tower_defence::Tile;
using antwika::tower_defence::TowerPlacementSink;
using ::testing::NiceMock;
using ::testing::ReturnRef;

namespace
{
    constexpr Size kCanvas{.width = 960, .height = 720};

    TickEvent tick()
    {
        return TickEvent{
            .tick = 0,
            .event = Event{.name = antwika::engine::events::kTick}};
    }

    TickEvent other()
    {
        return TickEvent{
            .tick = 0, .event = Event{.name = "something.else"}};
    }

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

    TickEvent pressAt(
        const InputEventCodec &codec,
        const std::int32_t x,
        const std::int32_t y,
        const MouseButton button = MouseButton::Left)
    {
        return TickEvent{
            .tick = 0,
            .event = codec.encode(PointerButtonPressed{
                .button = button, .position = {.x = x, .y = y}})};
    }

    TEST(BattleSinkTest, OnlyATickStepsTheBattle)
    {
        Battle battle(
            straightLevel(6), BattleConfig{.spawnPeriodTicks = 1});
        BattleSink sink(battle);

        sink.handle(other());
        EXPECT_EQ(battle.ticks(), 0U);

        sink.handle(tick());
        EXPECT_EQ(battle.ticks(), 1U);
    }

    TEST(ScoreSinkTest, OnlyATickDescribesTheBar)
    {
        Battle battle(straightLevel(6), BattleConfig{});
        ScoreOverlay overlay(kCanvas);
        ScoreSink sink(battle, overlay);

        sink.handle(other());
        EXPECT_TRUE(overlay.commands().empty());

        sink.handle(tick());
        EXPECT_FALSE(overlay.commands().empty());
    }

    TEST(TowerPlacementSinkTest, ALeftPressOnOpenGroundBuilds)
    {
        Battle battle(straightLevel(6), BattleConfig{});
        const InputEventCodec codec;
        TowerPlacementSink sink(battle, codec, kCanvas);

        const auto layout = layoutFor(kCanvas, 6, 3);
        ASSERT_TRUE(layout.has_value());
        const auto rect = cellRect(*layout, {.x = 2, .y = 1});

        sink.handle(pressAt(codec, rect.origin.x + 4, rect.origin.y + 4));
        ASSERT_EQ(battle.towers().size(), 1U);
        EXPECT_EQ(battle.towers()[0].cell, (Cell{.x = 2, .y = 1}));
    }

    TEST(TowerPlacementSinkTest, APressOnTheRunBuildsNothing)
    {
        Battle battle(straightLevel(6), BattleConfig{});
        const InputEventCodec codec;
        TowerPlacementSink sink(battle, codec, kCanvas);

        const auto layout = layoutFor(kCanvas, 6, 3);
        ASSERT_TRUE(layout.has_value());
        const auto rect = cellRect(*layout, {.x = 2, .y = 0});

        sink.handle(pressAt(codec, rect.origin.x + 4, rect.origin.y + 4));
        EXPECT_TRUE(battle.towers().empty());
    }

    TEST(TowerPlacementSinkTest, APressOnTheScoreBarBuildsNothing)
    {
        Battle battle(straightLevel(6), BattleConfig{});
        const InputEventCodec codec;
        TowerPlacementSink sink(battle, codec, kCanvas);

        sink.handle(pressAt(codec, 100, 4));
        EXPECT_TRUE(battle.towers().empty());
    }

    TEST(TowerPlacementSinkTest, OnlyALeftPressBuilds)
    {
        Battle battle(straightLevel(6), BattleConfig{});
        const InputEventCodec codec;
        TowerPlacementSink sink(battle, codec, kCanvas);

        const auto layout = layoutFor(kCanvas, 6, 3);
        ASSERT_TRUE(layout.has_value());
        const auto rect = cellRect(*layout, {.x = 2, .y = 1});

        sink.handle(pressAt(
            codec,
            rect.origin.x + 4,
            rect.origin.y + 4,
            MouseButton::Right));
        sink.handle(TickEvent{
            .tick = 0,
            .event = codec.encode(PointerButtonReleased{
                .button = MouseButton::Left,
                .position = {
                    .x = rect.origin.x + 4, .y = rect.origin.y + 4}})});
        sink.handle(tick());
        EXPECT_TRUE(battle.towers().empty());
    }

    TEST(TowerPlacementSinkTest, ACanvasWithNoRoomBuildsNothing)
    {
        Battle battle(straightLevel(6), BattleConfig{});
        const InputEventCodec codec;
        TowerPlacementSink sink(battle, codec, {.width = 4, .height = 4});

        sink.handle(pressAt(codec, 1, 1));
        EXPECT_TRUE(battle.towers().empty());
    }

    TEST(RenderSinkTest, ATickDrawsAFrameAndPacesIt)
    {
        Battle battle(straightLevel(6), BattleConfig{});
        ScoreOverlay overlay(kCanvas);
        const BattleScene scene;
        FakeSleeper sleeper;
        NiceMock<MockRenderer> renderer;
        NiceMock<MockWindow> window;
        ON_CALL(window, isOpen()).WillByDefault(::testing::Return(true));
        ON_CALL(window, renderer()).WillByDefault(ReturnRef(renderer));

        EXPECT_CALL(renderer, present()).Times(1);

        RenderSink sink(
            window,
            scene,
            battle,
            overlay,
            sleeper,
            std::chrono::milliseconds{5},
            kCanvas);
        sink.handle(tick());
        EXPECT_EQ(sleeper.requested().size(), 1U);
    }

    TEST(RenderSinkTest, AClosedWindowAndANonTickDrawNothing)
    {
        Battle battle(straightLevel(6), BattleConfig{});
        ScoreOverlay overlay(kCanvas);
        const BattleScene scene;
        FakeSleeper sleeper;
        NiceMock<MockWindow> window;
        ON_CALL(window, isOpen()).WillByDefault(::testing::Return(false));

        EXPECT_CALL(window, renderer()).Times(0);

        RenderSink sink(
            window,
            scene,
            battle,
            overlay,
            sleeper,
            std::chrono::milliseconds{5},
            kCanvas);
        sink.handle(other());
        sink.handle(tick());
        EXPECT_TRUE(sleeper.requested().empty());
    }
} // namespace
