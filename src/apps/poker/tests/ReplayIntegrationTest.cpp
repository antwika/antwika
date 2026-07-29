#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <sstream>
#include <string>
#include <vector>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/EventRecorder.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/event/TickEventRecorder.hpp>
#include <antwika/holdem/Blinds.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/log/MinimumLevelLogPolicy.hpp>
#include <antwika/log/NullAppender.hpp>
#include <antwika/log/PlainFormatter.hpp>
#include <antwika/replay/IReplaySource.hpp>
#include <antwika/replay/ReplayReader.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/replay/ReplayWriter.hpp>
#include <antwika/time/fakes/FakeClock.hpp>

#include "antwika/poker/Events.hpp"
#include "antwika/poker/PokerRoom.hpp"
#include "antwika/poker/RoomConfig.hpp"
#include "antwika/poker/RoomSummary.hpp"

using antwika::event::Event;
using antwika::event::EventRecorder;
using antwika::event::TickEvent;
using antwika::event::TickEventRecorder;
using antwika::holdem::Blinds;
using antwika::log::Level;
using antwika::log::MinimumLevelLogPolicy;
using antwika::log::NullAppender;
using antwika::log::PlainFormatter;
using antwika::replay::IReplaySource;
using antwika::replay::ReplayReader;
using antwika::replay::ReplaySource;
using antwika::replay::ReplayWriter;
using antwika::poker::RoomConfig;
using antwika::poker::RoomSummary;
using antwika::time::fakes::FakeClock;

namespace
{
    constexpr antwika::time::Tick kMaxTicks = 400;

    constexpr RoomConfig kRoom{
        .seatCount = 4,
        .blinds = Blinds{.small = 5, .big = 10},
        .minimumBuyIn = 100,
        .seed = 4242,
    };

    struct Session
    {
        RoomSummary summary;
        std::string narration;
        std::vector<TickEvent> recorded;

        bool operator==(const Session &other) const = default;
    };

    [[nodiscard]] Session runSession(IReplaySource &source)
    {
        std::chrono::system_clock::time_point time{};
        FakeClock clock(time);
        NullAppender appender;
        PlainFormatter formatter;
        MinimumLevelLogPolicy logPolicy(Level::Warning);
        EventRecorder eventSink;
        TickEventRecorder replayRecorder;
        std::ostringstream out;

        auto summary = antwika::poker::bootstrap(
            clock,
            appender,
            formatter,
            logPolicy,
            eventSink,
            source,
            out,
            kRoom,
            kMaxTicks,
            &replayRecorder);

        return Session{
            .summary = std::move(summary),
            .narration = out.str(),
            .recorded = replayRecorder.getEvents(),
        };
    }

    [[nodiscard]] TickEvent at(
        antwika::time::Tick tick, const char *name, std::string payload)
    {
        return TickEvent{
            .tick = tick,
            .event = Event{.name = name, .payload = std::move(payload)},
        };
    }

    [[nodiscard]] std::vector<TickEvent> liveScript()
    {
        std::vector<TickEvent> script;
        for (const auto *player : {"alice", "bob", "carol"})
        {
            script.push_back(at(
                0,
                antwika::poker::events::kDeposit,
                std::string(R"({"player":")") + player
                    + R"(","amount":800})"));
            script.push_back(at(
                0,
                antwika::poker::events::kBuyIn,
                std::string(R"({"player":")") + player
                    + R"(","amount":300})"));
        }
        // Somebody walks in later, and somebody else walks out.
        script.push_back(at(
            2,
            antwika::poker::events::kDeposit,
            R"({"player":"dave","amount":800})"));
        script.push_back(at(
            60,
            antwika::poker::events::kBuyIn,
            R"({"player":"dave","amount":200})"));
        script.push_back(at(200, antwika::engine::events::kStop, ""));
        return script;
    }
} // namespace

// This is the requirement this project exists for, applied to poker.
// Save a replay from a live run, then load it back.
// The second session plays out identically, down to the chip counts.
// Not one card and not one action is stored in that replay.
// Who walked in with what money is the only thing it holds.
// Everything else follows from the seed and the agents' policies.
TEST(ReplayIntegrationTest, LoadingASavedReplayReproducesTheSameSession)
{
    const auto script = liveScript();

    ReplaySource liveSource(script);
    const auto live = runSession(liveSource);

    ReplayWriter writer;
    std::stringstream replayStream;
    writer.write(script, replayStream);

    ReplayReader reader;
    auto loadedEvents = reader.read(replayStream);
    ReplaySource replaySource(loadedEvents);
    const auto replayed = runSession(replaySource);

    EXPECT_EQ(replayed, live);
    EXPECT_GT(live.summary.handsPlayed, 5U);
}

// Only the money coming in and out was ever external input.
// Nothing the engine or the agents generate belongs in a replay.
// So filtering those out leaves exactly the script it was driven with.
TEST(ReplayIntegrationTest, RecordedEventsHoldOnlyTheRoomsOwnInput)
{
    const auto script = liveScript();
    ReplaySource source(script);

    auto recorded = runSession(source).recorded;
    std::erase_if(
        recorded,
        [](const TickEvent &event)
        {
            return event.event.name == antwika::engine::events::kTick
                   || event.event.name == "Running Antwika Poker";
        });

    EXPECT_EQ(recorded, script);
}
