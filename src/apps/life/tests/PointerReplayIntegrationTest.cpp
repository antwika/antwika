#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/mocks/MockEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/event/TickEventRecorder.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/Events.hpp>
#include <antwika/input/IdleMotionSource.hpp>
#include <antwika/input/InputCapabilities.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/LiveInputSource.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/input/Position.hpp>
#include <antwika/input/fakes/FakeInputBackend.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/replay/ReplayCli.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/testing/ScratchPath.hpp>

#include "antwika/life/Board.hpp"
#include "antwika/life/Events.hpp"
#include "antwika/life/DragState.hpp"
#include "antwika/life/Grid.hpp"
#include "antwika/life/Life.hpp"
#include "antwika/life/PointerToggleSink.hpp"

using antwika::ecs::ISystem;
using antwika::ecs::World;
using antwika::event::Event;
using antwika::event::mocks::MockEventSink;
using antwika::event::TickEvent;
using antwika::event::TickEventRecorder;
using antwika::gfx::Size;
using antwika::input::IdleMotionSource;
using antwika::input::InputCapabilities;
using antwika::input::InputEvent;
using antwika::input::InputEventCodec;
using antwika::input::LiveInputSource;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::input::PointerButtonReleased;
using antwika::input::PointerMoved;
using antwika::input::Position;
using antwika::input::fakes::FakeInputBackend;
using antwika::life::Board;
using antwika::life::DragState;
using antwika::life::readBoardFromView;
using antwika::life::Grid;
using antwika::life::PointerToggleSink;
using antwika::log::mocks::MockLogger;
using antwika::replay::ReplaySource;
using ::testing::NiceMock;

namespace
{
    constexpr std::uint32_t kWidth = 8;
    constexpr std::uint32_t kHeight = 8;
    constexpr antwika::time::Tick kMaxTicks = 8;

    constexpr antwika::time::Tick kStopTick = 6;

    constexpr Size kCanvas{.width = 80, .height = 80};

    constexpr InputCapabilities kPointerOnly{
        .keyboard = false, .pointer = true};

    class FakeBoardRecorder final : public ISystem
    {
    public:
        void update(World &world, antwika::time::Tick) override
        {
            boards.push_back(readBoardFromView(world, kWidth, kHeight));
        }

        [[nodiscard]] const std::vector<Board> &perTick() const noexcept
        {
            return boards;
        }

    private:
        std::vector<Board> boards;
    };

    [[nodiscard]] InputEvent pressAt(std::int32_t x, std::int32_t y)
    {
        return PointerButtonPressed{
            .button = MouseButton::Left, .position = Position{.x = x, .y = y}};
    }

    [[nodiscard]] InputEvent moveTo(std::int32_t x, std::int32_t y)
    {
        return PointerMoved{.position = Position{.x = x, .y = y}};
    }

    [[nodiscard]] InputEvent releaseAt(std::int32_t x, std::int32_t y)
    {
        return PointerButtonReleased{
            .button = MouseButton::Left, .position = Position{.x = x, .y = y}};
    }

    [[nodiscard]] std::vector<std::vector<InputEvent>> dragRounds()
    {
        return {
            {pressAt(15, 15),
             moveTo(25, 15),
             moveTo(25, 25),
             moveTo(15, 25),
             releaseAt(15, 25)},
            {},
            {pressAt(55, 55)},
            {moveTo(65, 55)},
            {releaseAt(65, 55)},
        };
    }

    [[nodiscard]] std::vector<TickEvent> stopAt(antwika::time::Tick tick)
    {
        return {TickEvent{
            .tick = tick,
            .event = Event{.name = antwika::engine::events::kStop}}};
    }

    [[nodiscard]] antwika::life::TickSinkFactory toggleSinkFactory(
        const InputEventCodec &codec)
    {
        return [&codec](World &world, const Grid &grid, DragState &drag)
        {
            return std::make_unique<PointerToggleSink>(
                world, grid, codec, kCanvas, drag);
        };
    }

    [[nodiscard]] bool anythingAlive(const Board &board)
    {
        for (const bool alive : board.alive)
        {
            if (alive)
            {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] std::size_t countNamed(
        const std::vector<TickEvent> &events, std::string_view name)
    {
        std::size_t found = 0;
        for (const auto &event : events)
        {
            if (event.event.name == name)
            {
                ++found;
            }
        }
        return found;
    }
}

TEST(PointerReplayIntegrationTest, Replay_ReachesTheSameBoardAfterADrag)
{
    const antwika::testing::ScratchFile replayFile(
        "antwika_life_pointer_drag.replay");
    const InputEventCodec codec;

    antwika::life::LifeSummary liveBoard;
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> eventSink;
        TickEventRecorder replayRecorder;

        FakeInputBackend backend(dragRounds(), kPointerOnly);
        ReplaySource fileSource(stopAt(kStopTick));
        LiveInputSource source(fileSource, backend, codec);

        liveBoard = antwika::life::bootstrap(
            antwika::life::LifeWiring{
                .logger = logger,
                .eventSink = eventSink,
                .inputSource = source,
                .width = kWidth,
                .height = kHeight,
                .maxTicks = kMaxTicks,
                .replayRecorder = replayRecorder,
                .extraSink = toggleSinkFactory(codec)});

        antwika::replay::saveReplayFile(
            replayRecorder.getEvents(), replayFile.string());
    }

    ASSERT_TRUE(anythingAlive(liveBoard.board));

    const auto recorded =
        antwika::replay::loadReplayFile(replayFile.string());

    EXPECT_EQ(countNamed(recorded, antwika::input::events::kPointerDown), 2u);
    EXPECT_EQ(countNamed(recorded, antwika::input::events::kPointerMove), 4u);
    EXPECT_EQ(countNamed(recorded, antwika::input::events::kPointerUp), 2u);
    EXPECT_EQ(
        countNamed(recorded, antwika::life::events::kToggleCell), 0u);

    NiceMock<MockLogger> logger;
    NiceMock<MockEventSink> eventSink;
    ReplaySource replaySource(recorded);

    const auto replayedBoard = antwika::life::bootstrap(
        antwika::life::LifeWiring{
            .logger = logger,
            .eventSink = eventSink,
            .inputSource = replaySource,
            .width = kWidth,
            .height = kHeight,
            .maxTicks = kMaxTicks,
            .extraSink = toggleSinkFactory(codec)});

    EXPECT_EQ(replayedBoard, liveBoard);
}

TEST(PointerReplayIntegrationTest, Run_DrawsWhatADragCrossedInOneTick)
{
    const InputEventCodec codec;

    NiceMock<MockLogger> logger;
    NiceMock<MockEventSink> eventSink;

    FakeInputBackend backend(
        {pressAt(15, 15),
         moveTo(25, 15),
         moveTo(25, 25),
         moveTo(15, 25),
         releaseAt(15, 25)},
        kPointerOnly);

    ReplaySource fileSource(stopAt(0));
    LiveInputSource source(fileSource, backend, codec);

    const auto board = antwika::life::bootstrap(
        antwika::life::LifeWiring{
            .logger = logger,
            .eventSink = eventSink,
            .inputSource = source,
            .width = kWidth,
            .height = kHeight,
            .maxTicks = 2,
            .extraSink = toggleSinkFactory(codec)}).board;

    for (std::uint32_t y = 0; y < kHeight; ++y)
    {
        for (std::uint32_t x = 0; x < kWidth; ++x)
        {
            const bool expected = (x == 1 || x == 2) && (y == 1 || y == 2);
            const auto index = static_cast<std::size_t>(y) * kWidth + x;

            EXPECT_EQ(board.alive[index], expected)
                << x << ' ' << y;
        }
    }
}

TEST(PointerReplayIntegrationTest, Run_StopsTheGenerationsWhileHeld)
{
    const InputEventCodec codec;

    NiceMock<MockLogger> logger;
    NiceMock<MockEventSink> eventSink;
    FakeBoardRecorder recorder;

    std::vector<TickEvent> script{
        TickEvent{
            .tick = 0,
            .event = Event{
                .name = antwika::life::events::kToggleCell,
                .payload = R"({"x":1,"y":1})"}},
        TickEvent{
            .tick = 0,
            .event = Event{
                .name = antwika::life::events::kToggleCell,
                .payload = R"({"x":2,"y":1})"}},
        TickEvent{
            .tick = 0,
            .event = Event{
                .name = antwika::life::events::kToggleCell,
                .payload = R"({"x":3,"y":1})"}},
        TickEvent{
            .tick = 6,
            .event = Event{.name = antwika::engine::events::kStop}},
    };

    FakeInputBackend backend(
        std::vector<std::vector<InputEvent>>{
            {},
            {},
            {pressAt(-50, -50)},
            {},
            {},
            {releaseAt(-50, -50)},
        },
        kPointerOnly);

    ReplaySource fileSource(script);
    LiveInputSource source(fileSource, backend, codec);

    std::vector<std::reference_wrapper<ISystem>> observers{recorder};

    antwika::life::bootstrap(
        antwika::life::LifeWiring{
            .logger = logger,
            .eventSink = eventSink,
            .inputSource = source,
            .width = kWidth,
            .height = kHeight,
            .observers = observers,
            .maxTicks = kMaxTicks,
            .extraSink = toggleSinkFactory(codec)});

    const auto &boards = recorder.perTick();
    ASSERT_EQ(boards.size(), 7u);

    ASSERT_TRUE(anythingAlive(boards[1]));

    EXPECT_NE(boards[0], boards[1]);

    EXPECT_EQ(boards[2], boards[1]);
    EXPECT_EQ(boards[3], boards[2]);
    EXPECT_EQ(boards[4], boards[3]);

    EXPECT_NE(boards[5], boards[4]);
    EXPECT_EQ(boards[5], boards[0]);
}

namespace
{
    [[nodiscard]] std::vector<std::vector<InputEvent>> wanderingDragRounds()
    {
        return {
            {moveTo(5, 5), moveTo(15, 5), moveTo(15, 15)},
            {pressAt(15, 15),
             moveTo(25, 15),
             moveTo(25, 25),
             moveTo(15, 25),
             releaseAt(15, 25)},
            {moveTo(35, 35), moveTo(45, 45)},
            {pressAt(55, 55)},
            {moveTo(65, 55)},
            {releaseAt(65, 55)},
        };
    }

    struct WanderResult final
    {
        Board board;
        std::vector<TickEvent> recorded;
    };

    [[nodiscard]] WanderResult runWandering(
        const InputEventCodec &codec, bool gate)
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> eventSink;
        TickEventRecorder replayRecorder;

        FakeInputBackend backend(wanderingDragRounds(), kPointerOnly);
        ReplaySource fileSource(stopAt(kStopTick));
        LiveInputSource live(fileSource, backend, codec);
        IdleMotionSource gated(live, codec);

        antwika::event::ITickEventSource &source =
            gate ? static_cast<antwika::event::ITickEventSource &>(gated)
                 : static_cast<antwika::event::ITickEventSource &>(live);

        auto summary = antwika::life::bootstrap(
            antwika::life::LifeWiring{
                .logger = logger,
                .eventSink = eventSink,
                .inputSource = source,
                .width = kWidth,
                .height = kHeight,
                .maxTicks = kMaxTicks,
                .replayRecorder = replayRecorder,
                .extraSink = toggleSinkFactory(codec)});

        return WanderResult{
            .board = std::move(summary.board),
            .recorded = replayRecorder.getEvents()};
    }
}

TEST(PointerReplayIntegrationTest, Run_LeavesTheSameBoardWhenGated)
{
    const InputEventCodec codec;

    const auto ungated = runWandering(codec, false);
    const auto gated = runWandering(codec, true);

    EXPECT_EQ(gated.board.alive, ungated.board.alive);
    EXPECT_TRUE(anythingAlive(gated.board));

    EXPECT_EQ(
        countNamed(ungated.recorded, antwika::input::events::kPointerMove),
        9U);
    EXPECT_EQ(
        countNamed(gated.recorded, antwika::input::events::kPointerMove),
        6U);
}
