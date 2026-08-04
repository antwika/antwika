#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <antwika/console/ConsolePicture.hpp>
#include <antwika/console/ConsoleState.hpp>
#include <antwika/console/SnapshotFormat.hpp>
#include <antwika/console/conformance/ConsoleContract.hpp>
#include <antwika/console/conformance/ConsoleSnapshotRoundTrip.hpp>
#include <antwika/console/testing/ConsoleScript.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/event/mocks/MockEventSink.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/testing/ScratchPath.hpp>

#include "antwika/life/Board.hpp"
#include "antwika/life/Events.hpp"
#include "antwika/life/Life.hpp"
#include "antwika/life/PointerToggleSink.hpp"
#include "antwika/life/StateDump.hpp"

using antwika::console::kConsoleAnimTicks;
using antwika::console::testing::keyAt;
using antwika::console::testing::kOpenTick;
using antwika::console::testing::pressAt;
using antwika::console::testing::stopAt;
using antwika::console::testing::typeText;
using antwika::ecs::World;
using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::event::mocks::MockEventSink;
using antwika::input::InputEventCodec;
using antwika::input::Key;
using antwika::life::Board;
using antwika::life::DragState;
using antwika::life::Grid;
using antwika::life::PointerToggleSink;
using antwika::log::mocks::MockLogger;
using antwika::replay::ReplaySource;
using antwika::time::Tick;
using ::testing::NiceMock;

namespace
{
    constexpr std::uint32_t kWidth = 4;
    constexpr std::uint32_t kHeight = 4;

    // Ten whole pixels per cell, so the sheet covers rows 0 and 1.
    constexpr antwika::gfx::Size kCanvas{.width = 40, .height = 40};

    [[nodiscard]] TickEvent toggleAt(
        Tick tick, std::uint32_t x, std::uint32_t y)
    {
        return TickEvent{
            .tick = tick,
            .event = Event{
                .name = antwika::life::events::kToggleCell,
                .payload = R"({"x":)" + std::to_string(x)
                           + R"(,"y":)" + std::to_string(y) + "}"}};
    }

    // A 2x2 block: a still life, so the generations leave it alone.
    void seedBlock(std::vector<TickEvent> &events, Tick tick)
    {
        events.push_back(toggleAt(tick, 1, 1));
        events.push_back(toggleAt(tick, 2, 1));
        events.push_back(toggleAt(tick, 1, 2));
        events.push_back(toggleAt(tick, 2, 2));
    }

    [[nodiscard]] std::vector<bool> blockOn4x4()
    {
        std::vector<bool> alive(16, false);
        alive[1 * 4 + 1] = true;
        alive[1 * 4 + 2] = true;
        alive[2 * 4 + 1] = true;
        alive[2 * 4 + 2] = true;
        return alive;
    }

    // A dump read back through this application's own envelope.
    struct ReadDump
    {
        antwika::life::StateDump state;
        std::vector<std::string> console;
    };

    [[nodiscard]] ReadDump readDump(const std::string &path)
    {
        const antwika::console::SnapshotFormat format(
            {.magic = antwika::life::kStateDumpMagic,
             .version = antwika::life::kStateDumpVersion},
            "antwika life state dump document",
            antwika::life::standardStateDumpMigrations);

        const auto snapshot = format.read(path);

        return ReadDump{
            .state = antwika::life::stateDumpFromJson(snapshot.state),
            .console = snapshot.console};
    }

    // BootstrapTest's harness, with the console turned on.
    struct ConsoleHarness
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> eventSink;
        InputEventCodec codec;
        antwika::console::ConsolePicture consoleOverlay{kCanvas};

        antwika::life::LifeSummary run(
            ReplaySource &source,
            Tick maxTicks,
            const std::string &dumpPath,
            bool loadEnabled = true)
        {
            return antwika::life::bootstrap(antwika::life::LifeWiring{
                .logger = logger,
                .eventSink = eventSink,
                .inputSource = source,
                .width = kWidth,
                .height = kHeight,
                .maxTicks = maxTicks,
                .extraSink =
                    [this](
                        World &world,
                        const Grid &grid,
                        DragState &drag)
                {
                    return std::make_unique<PointerToggleSink>(
                        world, grid, codec, kCanvas, drag);
                },
                .consoleOverlay = consoleOverlay,
                .consoleLoadEnabled = loadEnabled,
                .stateDumpPath = dumpPath});
        }
    };

    // This application's half of the shared console contract.
    struct LifeConsoleTraits
    {
        using Summary = antwika::life::LifeSummary;

        static Summary run(
            std::vector<TickEvent> script,
            const std::string &dumpPath,
            const bool loadEnabled)
        {
            script.push_back(stopAt(kOpenTick + 1));

            ReplaySource source(std::move(script));
            ConsoleHarness harness;

            return harness.run(source, 40, dumpPath, loadEnabled);
        }

        static const std::vector<std::string> &console(
            const Summary &summary)
        {
            return summary.console;
        }

        static void expectUntouched(const Summary &summary)
        {
            EXPECT_EQ(
                summary.board.alive, std::vector<bool>(16, false));
        }

        static std::string scratchPrefix()
        {
            return "antwika_life_console.";
        }
    };
} // namespace

namespace antwika::console::conformance
{

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Life, ConsoleContract, LifeConsoleTraits);

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Life, ConsoleSnapshotRoundTrip, LifeConsoleTraits);

} // namespace antwika::console::conformance

TEST(ConsoleSinkTest, APressUnderTheSheetTogglesNoCell)
{
    ConsoleHarness harness;
    std::vector<TickEvent> events{keyAt(harness.codec, 1, Key::Grave)};

    // One press under the sheet and one below it.
    // Only the second may reach the board.
    events.push_back(
        pressAt(harness.codec, kOpenTick, {.x = 5, .y = 5}));
    events.push_back(
        pressAt(harness.codec, kOpenTick, {.x = 5, .y = 35}));
    events.push_back(stopAt(kOpenTick + 1));
    ReplaySource source(std::move(events));

    const auto summary = harness.run(source, 40, "unused.json");

    // The press below the sheet started a drag, and a drag pauses.
    // So the lone cell it toggled is still there.
    std::vector<bool> expected(16, false);
    expected[3 * 4 + 0] = true;
    EXPECT_EQ(summary.board.alive, expected);
}

TEST(ConsoleSinkTest, DumpStateWritesTheInstantAndSaysSo)
{
    const antwika::testing::ScratchFile file(
        "antwika_life_console_dump.json");
    const auto path = file.path().string();

    ConsoleHarness harness;
    std::vector<TickEvent> events;
    seedBlock(events, 1);
    events.push_back(keyAt(harness.codec, 2, Key::Grave));
    typeText(
        events, harness.codec, 2 + kConsoleAnimTicks, "dump_state");
    events.push_back(
        keyAt(harness.codec, 2 + kConsoleAnimTicks, Key::Enter));
    events.push_back(stopAt(3 + kConsoleAnimTicks));
    ReplaySource source(std::move(events));

    const auto summary = harness.run(source, 40, path);

    const auto dumped = readDump(path);

    EXPECT_EQ(dumped.state.board.alive, blockOn4x4());
    EXPECT_FALSE(dumped.state.dragging);
    EXPECT_TRUE(dumped.state.visited.empty());
    EXPECT_EQ(dumped.state.lastDrag, std::nullopt);
    EXPECT_EQ(
        dumped.console,
        (std::vector<std::string>{
            "> dump_state", "dumped state to " + path}));
    EXPECT_EQ(summary.console, dumped.console);
}

TEST(ConsoleSinkTest, LoadStateComesBackToTheDumpedBoard)
{
    const antwika::testing::ScratchFile file(
        "antwika_life_console_load.json");
    const auto path = file.path().string();

    // One session draws a still life, then dumps itself.
    {
        ConsoleHarness harness;
        std::vector<TickEvent> events;
        seedBlock(events, 1);
        events.push_back(keyAt(harness.codec, 2, Key::Grave));
        typeText(
            events, harness.codec, 2 + kConsoleAnimTicks,
            "dump_state");
        events.push_back(
            keyAt(harness.codec, 2 + kConsoleAnimTicks, Key::Enter));
        events.push_back(stopAt(3 + kConsoleAnimTicks));
        ReplaySource source(std::move(events));

        harness.run(source, 40, path);
    }

    const auto dumped = readDump(path);

    // A fresh session loads it and continues from that board.
    ConsoleHarness fresh;
    std::vector<TickEvent> events{keyAt(fresh.codec, 1, Key::Grave)};
    typeText(events, fresh.codec, kOpenTick, "load_state");
    events.push_back(keyAt(fresh.codec, kOpenTick, Key::Enter));
    events.push_back(stopAt(kOpenTick + 1));
    ReplaySource source(std::move(events));

    const auto summary = fresh.run(source, 40, path);

    // The console history the load carried is the contract's.
    // The board it came back to is this file's.
    EXPECT_EQ(summary.board.alive, dumped.state.board.alive);
}
