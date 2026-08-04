#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

#include <antwika/console/ConsolePicture.hpp>
#include <antwika/console/conformance/ConsoleContract.hpp>
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

    // This application's half of the shared console contract.
    //
    // It instantiates ConsoleContract and not the round trip.
    // The room dumps itself mid-hand and reads no dump back.
    // So there is no round trip here to hold to that promise.
    struct PokerConsoleTraits
    {
        using Summary = antwika::poker::RoomSummary;

        static Summary run(
            std::vector<TickEvent> script,
            const std::string &dumpPath,
            const bool loadEnabled)
        {
            return ::run(std::move(script), dumpPath, loadEnabled);
        }

        static const std::vector<std::string> &console(
            const Summary &summary)
        {
            return summary.console;
        }

        // A refused load leaves a hand nothing here can name.
        // The room is dealt from a seed, so every number is the deal's.
        static void expectUntouched(const Summary &)
        {
        }

        static std::string scratchPrefix()
        {
            return "antwika_poker_console.";
        }
    };
} // namespace

namespace antwika::console::conformance
{

    INSTANTIATE_TYPED_TEST_SUITE_P(
        Poker, ConsoleContract, PokerConsoleTraits);

} // namespace antwika::console::conformance

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
