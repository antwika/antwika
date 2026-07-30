#include "antwika/poker/PokerRoom.hpp"

#include <array>
#include <iostream>
#include <string>
#include <string_view>

#include <antwika/event/EventRecorder.hpp>
#include <antwika/event/TickEventRecorder.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/log/MinimumLevelLogPolicy.hpp>
#include <antwika/log/PlainFormatter.hpp>
#include <antwika/log/StreamAppender.hpp>
#include <antwika/replay/ReplayCli.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/time/SystemClock.hpp>

#include "antwika/poker/RoomConfig.hpp"

using antwika::event::EventRecorder;
using antwika::event::TickEventRecorder;
using antwika::log::Level;
using antwika::log::MinimumLevelLogPolicy;
using antwika::log::PlainFormatter;
using antwika::log::StreamAppender;
using antwika::replay::ReplaySource;
using antwika::time::SystemClock;

namespace
{
    constexpr std::string_view kDemoReplayPath = ANTWIKA_POKER_DEMO_REPLAY_PATH;

    constexpr std::array<std::string_view, 1> kSelfGeneratedEventNames{
        "Running Antwika Poker",
    };
} // namespace

int main(int argc, char **argv)
{
    const auto options = antwika::replay::parseReplayCliOptions(argc, argv);

    SystemClock clock;
    StreamAppender appender(std::cout);
    PlainFormatter formatter;
    MinimumLevelLogPolicy logPolicy(Level::Warning);
    EventRecorder eventSink;
    TickEventRecorder replayRecorder;

    auto events = antwika::replay::loadReplayFile(
        options.replayPath.value_or(std::string(kDemoReplayPath)));
    ReplaySource source(std::move(events));

    const auto summary = antwika::poker::bootstrap(
        clock,
        appender,
        formatter,
        logPolicy,
        eventSink,
        source,
        std::cout,
        antwika::poker::RoomConfig{},
        std::nullopt,
        &replayRecorder);

    std::cout << "\n=== " << summary.handsPlayed << " hands played ===\n";
    for (const auto &[player, balance] : summary.balances)
    {
        std::cout << "  " << player << ": " << balance << '\n';
    }
    if (summary.chipsLeftOnTable > 0)
    {
        std::cout << "  (" << summary.chipsLeftOnTable
                  << " chips left in an unfinished hand)\n";
    }

    if (options.recordPath)
    {
        antwika::replay::saveReplayFile(
            replayRecorder.getEvents(),
            *options.recordPath,
            kSelfGeneratedEventNames);
    }

    return 0;
}
