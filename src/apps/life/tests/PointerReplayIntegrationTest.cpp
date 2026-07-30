#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include <antwika/ecs/World.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/EventRecorder.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/event/TickEventRecorder.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/Events.hpp>
#include <antwika/input/IInputBackend.hpp>
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

#include "antwika/life/Board.hpp"
#include "antwika/life/Events.hpp"
#include "antwika/life/Grid.hpp"
#include "antwika/life/Life.hpp"
#include "antwika/life/PointerToggleSink.hpp"

using antwika::ecs::World;
using antwika::event::Event;
using antwika::event::EventRecorder;
using antwika::event::TickEvent;
using antwika::event::TickEventRecorder;
using antwika::gfx::Size;
using antwika::input::IInputBackend;
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

    // Nothing here closes a window.
    // So the run stops the way a replay does, on an engine.stop.
    // The tick it lands on still runs to completion.
    constexpr antwika::time::Tick kStopTick = 6;

    // Ten pixels a cell, so a coordinate is its cell with a five after.
    constexpr Size kCanvas{.width = 80, .height = 80};

    constexpr std::array<std::string_view, 1> kSelfGeneratedEventNames{
        antwika::life::events::kStarted,
    };

    /**
     * @brief IInputBackend reporting one tick's worth of edges per drain.
     *
     * FakeInputBackend hands over its whole script the first time it is
     * drained, which is one tick. A drag crossing tick boundaries is the
     * more interesting case -- it is what makes the sink's held state
     * something a replay has to regenerate rather than something that
     * lives and dies inside a single tick.
     */
    class RoundedInputBackend final : public IInputBackend
    {
    public:
        explicit RoundedInputBackend(
            std::vector<std::vector<InputEvent>> rounds)
            : rounds(std::move(rounds))
        {
        }

        [[nodiscard]] std::string_view name() const override
        {
            return "rounded";
        }

        [[nodiscard]] InputCapabilities capabilities() const override
        {
            return InputCapabilities{.keyboard = false, .pointer = true};
        }

        [[nodiscard]] std::optional<InputEvent> pollEvent() override
        {
            if (round >= rounds.size())
            {
                return std::nullopt;
            }

            if (next < rounds[round].size())
            {
                auto event = rounds[round][next];
                ++next;
                return event;
            }

            // Reporting an empty queue is what ends this tick's drain.
            ++round;
            next = 0;

            return std::nullopt;
        }

    private:
        std::vector<std::vector<InputEvent>> rounds;
        std::size_t round = 0;
        std::size_t next = 0;
    };

    // Removes its backing file on scope exit.
    class ScratchFile
    {
    public:
        explicit ScratchFile(std::string_view name)
            : path(std::filesystem::temp_directory_path() / name)
        {
        }

        ~ScratchFile()
        {
            std::error_code ignored;
            std::filesystem::remove(path, ignored);
        }

        ScratchFile(const ScratchFile &) = delete;
        ScratchFile(ScratchFile &&) = delete;
        ScratchFile &operator=(const ScratchFile &) = delete;
        ScratchFile &operator=(ScratchFile &&) = delete;

        [[nodiscard]] std::string string() const
        {
            return path.string();
        }

    private:
        std::filesystem::path path;
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

    /**
     * @brief The drag that draws a 2x2 block, then one that draws a pair.
     *
     * The block is drawn inside a single tick, so it is complete before
     * the first generation runs and survives every one after it -- which
     * is what keeps the board this test compares from being empty.
     */
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
        return [&codec](World &world, const Grid &grid)
        {
            return std::make_unique<PointerToggleSink>(
                world, grid, codec, kCanvas);
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
} // namespace

// The claim this whole feature rests on.
// Drag across a board with a live device, and record what it did.
// Then replay that recording with no device attached at all.
// The same board has to come back, or input found a second way in.
TEST(PointerReplayIntegrationTest, RecordingADragReplaysToTheSameBoard)
{
    const ScratchFile replayFile("antwika_life_pointer_drag.replay");
    const InputEventCodec codec;

    Board liveBoard;
    {
        NiceMock<MockLogger> logger;
        EventRecorder eventSink;
        TickEventRecorder replayRecorder;

        RoundedInputBackend backend(dragRounds());
        ReplaySource fileSource(stopAt(kStopTick));
        LiveInputSource source(fileSource, backend, codec);

        liveBoard = antwika::life::bootstrap(
            logger,
            eventSink,
            source,
            kWidth,
            kHeight,
            {},
            kMaxTicks,
            &replayRecorder,
            toggleSinkFactory(codec));

        // Through the real save, for the filtering main.cpp relies on.
        antwika::replay::saveReplayFile(
            replayRecorder.getEvents(),
            replayFile.string(),
            kSelfGeneratedEventNames);
    }

    ASSERT_TRUE(anythingAlive(liveBoard));

    const auto recorded =
        antwika::replay::loadReplayFile(replayFile.string());

    // What was persisted is the input, and only the input.
    // The toggles are derived from it downstream.
    // A replay storing them too would apply every one of them twice.
    EXPECT_EQ(countNamed(recorded, antwika::input::events::kPointerDown), 2u);
    EXPECT_EQ(countNamed(recorded, antwika::input::events::kPointerMove), 4u);
    EXPECT_EQ(countNamed(recorded, antwika::input::events::kPointerUp), 2u);
    EXPECT_EQ(
        countNamed(recorded, antwika::life::events::kToggleCell), 0u);

    // The replayed run: the recording, and no device.
    NiceMock<MockLogger> logger;
    EventRecorder eventSink;
    ReplaySource replaySource(recorded);

    const auto replayedBoard = antwika::life::bootstrap(
        logger,
        eventSink,
        replaySource,
        kWidth,
        kHeight,
        {},
        kMaxTicks,
        nullptr,
        toggleSinkFactory(codec));

    EXPECT_EQ(replayedBoard, liveBoard);
}

// The whole drag inside one tick.
// That is what a fast mouse and a slow tick amount to.
TEST(PointerReplayIntegrationTest, ADragWithinOneTickDrawsWhatItCrossed)
{
    const InputEventCodec codec;

    NiceMock<MockLogger> logger;
    EventRecorder eventSink;

    FakeInputBackend backend(
        {pressAt(15, 15),
         moveTo(25, 15),
         moveTo(25, 25),
         moveTo(15, 25),
         releaseAt(15, 25)},
        InputCapabilities{.keyboard = false, .pointer = true});

    ReplaySource fileSource(stopAt(0));
    LiveInputSource source(fileSource, backend, codec);

    // One tick only, so what is alive is what the drag drew.
    const auto board = antwika::life::bootstrap(
        logger,
        eventSink,
        source,
        kWidth,
        kHeight,
        {},
        2,
        nullptr,
        toggleSinkFactory(codec));

    // A 2x2 block is a still life.
    // So the generation that ran during that tick left it as drawn.
    for (std::uint32_t y = 0; y < kHeight; ++y)
    {
        for (std::uint32_t x = 0; x < kWidth; ++x)
        {
            const bool expected = (x == 1 || x == 2) && (y == 1 || y == 2);
            const auto index = static_cast<std::size_t>(y) * kWidth + x;

            EXPECT_EQ(board.alive[index], expected)
                << "at (" << x << ", " << y << ")";
        }
    }
}
