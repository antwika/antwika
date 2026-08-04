#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

#include <antwika/console/ConsolePicture.hpp>
#include <antwika/console/testing/ConsoleScript.hpp>
#include <antwika/event/mocks/MockEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/holdem/Blinds.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/testing/ScratchPath.hpp>
#include <antwika/time/fakes/FakeClock.hpp>

#include "antwika/poker/PokerRoom.hpp"
#include "antwika/poker/RoomConfig.hpp"

using antwika::console::testing::keyAt;
using antwika::console::testing::kOpenTick;
using antwika::console::testing::stopAt;
using antwika::console::testing::typeText;
using antwika::event::mocks::MockEventSink;
using antwika::event::TickEvent;
using antwika::holdem::Blinds;
using antwika::input::InputEventCodec;
using antwika::input::Key;
using antwika::log::mocks::MockLogger;
using antwika::replay::ReplaySource;
using antwika::time::fakes::FakeClock;
using ::testing::NiceMock;
using ::testing::StartsWith;

namespace
{
    constexpr antwika::poker::RoomConfig kRoom{
        .seatCount = 2,
        .blinds = Blinds{.small = 5, .big = 10},
        .minimumBuyIn = 100,
        .seed = 7,
    };

    [[nodiscard]] antwika::poker::RoomSummary run(
        std::vector<TickEvent> events,
        const std::string &dumpPath,
        bool loadEnabled)
    {
        events.push_back(stopAt(kOpenTick + 2));

        std::chrono::system_clock::time_point time{};
        FakeClock clock(time);
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> eventSink;
        InputEventCodec codec;
        ReplaySource source(std::move(events));
        antwika::console::ConsolePicture picture(
            {.width = 1024, .height = 640});
        std::ostringstream out;

        return antwika::poker::bootstrap(antwika::poker::RoomSetup{
            .clock = clock,
            .logger = logger,
            .eventSink = eventSink,
            .inputSource = source,
            .out = out,
            .room = kRoom,
            .codec = codec,
            .consoleOverlay = picture,
            .consoleLoadEnabled = loadEnabled,
            .stateDumpPath = dumpPath,
            .maxTicks = 100});
    }

} // namespace

TEST(ConsoleSinkTest, AnUnknownCommandIsEchoedAndRefused)
{
    InputEventCodec codec;
    std::vector<TickEvent> events{keyAt(codec, 1, Key::Grave)};
    typeText(events, codec, kOpenTick, "hello");
    events.push_back(keyAt(codec, kOpenTick, Key::Enter));

    const auto summary = run(std::move(events), "unused.json", true);

    EXPECT_EQ(
        summary.console,
        (std::vector<std::string>{
            "> hello", "unknown command: hello"}));
}

TEST(ConsoleSinkTest, DumpStateWritesTheRoomMidHand)
{
    const antwika::testing::ScratchFile file(
        "antwika_poker_console_dump.json");

    InputEventCodec codec;
    std::vector<TickEvent> events{keyAt(codec, 1, Key::Grave)};
    typeText(events, codec, kOpenTick, "dump_state");
    events.push_back(keyAt(codec, kOpenTick, Key::Enter));

    const auto summary =
        run(std::move(events), file.path().string(), true);

    EXPECT_EQ(
        summary.console,
        (std::vector<std::string>{
            "> dump_state",
            "dumped state to " + file.path().string()}));
    EXPECT_TRUE(std::filesystem::exists(file.path()));
}

TEST(ConsoleSinkTest, LoadStateIsRefusedWhileRecordingOrReplaying)
{
    InputEventCodec codec;
    std::vector<TickEvent> events{keyAt(codec, 1, Key::Grave)};
    typeText(events, codec, kOpenTick, "load_state");
    events.push_back(keyAt(codec, kOpenTick, Key::Enter));

    const auto summary = run(std::move(events), "unused.json", false);

    EXPECT_EQ(
        summary.console,
        (std::vector<std::string>{
            "> load_state",
            "load_state: not available while recording or "
            "replaying"}));
}

TEST(ConsoleSinkTest, LoadStateAnswersAFileThatIsNotThere)
{
    const antwika::testing::ScratchFile file(
        "antwika_poker_console_absent.json");

    InputEventCodec codec;
    std::vector<TickEvent> events{keyAt(codec, 1, Key::Grave)};
    typeText(events, codec, kOpenTick, "load_state");
    events.push_back(keyAt(codec, kOpenTick, Key::Enter));

    const auto summary =
        run(std::move(events), file.path().string(), true);

    ASSERT_EQ(summary.console.size(), 2U);
    EXPECT_THAT(summary.console[1], StartsWith("could not load: "));
}

TEST(ConsoleSinkTest, ARoomWithNoConsoleIgnoresTheKeys)
{
    std::chrono::system_clock::time_point time{};
    FakeClock clock(time);
    NiceMock<MockLogger> logger;
    NiceMock<MockEventSink> eventSink;
    InputEventCodec codec;

    std::vector<TickEvent> events{keyAt(codec, 1, Key::Grave)};
    events.push_back(stopAt(3));
    ReplaySource source(std::move(events));
    std::ostringstream out;

    const auto summary =
        antwika::poker::bootstrap(antwika::poker::RoomSetup{
            .clock = clock,
            .logger = logger,
            .eventSink = eventSink,
            .inputSource = source,
            .out = out,
            .room = kRoom,
            .maxTicks = 100});

    EXPECT_TRUE(summary.console.empty());
}
