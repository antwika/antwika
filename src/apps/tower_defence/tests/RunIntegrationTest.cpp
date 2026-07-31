#include <cstdint>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/mocks/MockEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/replay/ReplaySource.hpp>

#include "antwika/tower_defence/GridLayout.hpp"
#include "antwika/tower_defence/TowerDefence.hpp"

using antwika::event::Event;
using antwika::event::mocks::MockEventSink;
using antwika::event::TickEvent;
using antwika::gfx::Size;
using antwika::input::InputEventCodec;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::log::mocks::MockLogger;
using antwika::replay::ReplaySource;
using antwika::tower_defence::BattleConfig;
using antwika::tower_defence::BattleSummary;
using antwika::tower_defence::cellRect;
using antwika::tower_defence::layoutFor;
using antwika::tower_defence::LevelConfig;
using antwika::tower_defence::TowerDefenceConfig;
using ::testing::NiceMock;

namespace
{
    constexpr Size kCanvas{.width = 960, .height = 720};
    constexpr std::uint32_t kWidth = 12;
    constexpr std::uint32_t kHeight = 8;
    constexpr antwika::time::Tick kMaxTicks = 30;

    // The level is one simple path, so most of the grid is open ground.
    // The middle is a fair bet for a cell with no road on it.
    // The test asserts what happened rather than assuming it built.
    std::vector<TickEvent> script()
    {
        const InputEventCodec codec;
        const auto layout = layoutFor(kCanvas, kWidth, kHeight);
        const auto rect = cellRect(*layout, {.x = 6, .y = 4});

        return {
            TickEvent{
                .tick = 2,
                .event = codec.encode(PointerButtonPressed{
                    .button = MouseButton::Left,
                    .position = {
                        .x = rect.origin.x + 4,
                        .y = rect.origin.y + 4}})},
            TickEvent{
                .tick = 3,
                .event = codec.encode(PointerButtonPressed{
                    .button = MouseButton::Left,
                    .position = {.x = 20, .y = 8}})},
            TickEvent{
                .tick = 20,
                .event = Event{
                    .name = antwika::engine::events::kStop}}};
    }

    BattleSummary runOnce()
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> eventSink;
        const InputEventCodec codec;
        ReplaySource source(script());

        return antwika::tower_defence::bootstrap(TowerDefenceConfig{
            .logger = logger,
            .eventSink = eventSink,
            .inputSource = source,
            .codec = codec,
            .canvas = kCanvas,
            .level = LevelConfig{
                .width = kWidth, .height = kHeight, .seed = 3},
            .battle = BattleConfig{.spawnPeriodTicks = 3},
            .maxTicks = kMaxTicks});
    }
} // namespace

// The requirement this project exists for, for this app.
// A run is driven entirely by what a source hands it.
// Nothing about the level, the mobs or the towers is recorded.
// So running the same input twice has to land on the same state.
TEST(RunIntegrationTest, TheSameInputReachesTheSameStateTwice)
{
    const BattleSummary first = runOnce();
    const BattleSummary second = runOnce();

    EXPECT_EQ(first.score, second.score);
    EXPECT_EQ(first.leaks, second.leaks);
    EXPECT_EQ(first.ticks, second.ticks);
    EXPECT_EQ(first.towers, second.towers);
    EXPECT_EQ(first.pathLength, second.pathLength);
}

TEST(RunIntegrationTest, AStopEventEndsTheRunBeforeTheCap)
{
    const BattleSummary summary = runOnce();
    EXPECT_GT(summary.ticks, 0U);
    EXPECT_LT(summary.ticks, kMaxTicks);
    EXPECT_GT(summary.pathLength, 2U);
}

// One click landed on open ground and one on the score bar.
// Only the first can have built anything.
TEST(RunIntegrationTest, AClickOnTheBarBuildsNothingButAClickOnGroundDoes)
{
    const BattleSummary summary = runOnce();
    EXPECT_EQ(summary.towers, 1U);
}
