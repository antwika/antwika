#include "antwika/poker/PokerRoom.hpp"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>
#include <string_view>

#include <antwika/event/EventRecorder.hpp>
#include <antwika/event/TickEventRecorder.hpp>
#include <antwika/gfx/SelectedBackend.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/log/Logger.hpp>
#include <antwika/log/MinimumLevelLogPolicy.hpp>
#include <antwika/log/PlainFormatter.hpp>
#include <antwika/log/StreamAppender.hpp>
#include <antwika/replay/ReplayCli.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/time/SystemClock.hpp>
#include <antwika/time/SystemSleeper.hpp>

#include "antwika/poker/RoomConfig.hpp"
#include "antwika/poker/WatchOptions.hpp"
#include "antwika/poker/WindowSetup.hpp"

using antwika::event::EventRecorder;
using antwika::event::TickEventRecorder;
using antwika::log::Level;
using antwika::log::Logger;
using antwika::log::MinimumLevelLogPolicy;
using antwika::log::PlainFormatter;
using antwika::log::StreamAppender;
using antwika::poker::WindowSetup;
using antwika::replay::ReplaySource;
using antwika::time::SystemClock;
using antwika::time::SystemSleeper;

namespace
{
    constexpr std::string_view kDemoReplayPath = ANTWIKA_POKER_DEMO_REPLAY_PATH;
} // namespace

int main(int argc, char **argv)
{
    const auto options = antwika::replay::parseReplayCliOptions(argc, argv);
    const auto watch = antwika::poker::parseWatchOptions(argc, argv);

    SystemClock clock;
    StreamAppender appender(std::cout);
    PlainFormatter formatter;
    MinimumLevelLogPolicy logPolicy(Level::Warning);
    EventRecorder eventSink;
    TickEventRecorder replayRecorder;

    // Catching is what makes the run's resources unwind at all.
    // An uncaught exception may call std::terminate without unwinding.
    // Catching here also lets a failed --record run save what it has.
    int exitCode = EXIT_SUCCESS;
    try
    {
        auto events = antwika::replay::loadReplayFile(
            options.replayPath.value_or(std::string(kDemoReplayPath)));
        ReplaySource source(std::move(events));

        // The window is always opened, as in the gfx demo.
        // Under the headless backend it draws and costs nothing.
        Logger logger(formatter, logPolicy, clock, appender);
        const auto backend = antwika::gfx::makeSelectedBackend(logger);
        SystemSleeper sleeper;
        const WindowSetup window{
            .backend = *backend,
            .sleeper = sleeper,
            .framePeriod = watch.tickDelay,
        };

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
            &replayRecorder,
            &window);

        std::cout << "\n=== " << summary.handsPlayed
                  << " hands played ===\n";
        for (const auto &[player, balance] : summary.balances)
        {
            std::cout << "  " << player << ": " << balance << '\n';
        }
        if (summary.chipsLeftOnTable > 0)
        {
            std::cout << "  (" << summary.chipsLeftOnTable
                      << " chips left in an unfinished hand)\n";
        }
    }
    catch (const std::exception &error)
    {
        std::cerr << "antwika_poker: " << error.what() << '\n';
        exitCode = EXIT_FAILURE;
    }

    if (options.recordPath)
    {
        antwika::replay::saveReplayFile(
            replayRecorder.getEvents(), *options.recordPath);
    }

    // A run that threw now says so, rather than always reporting success.
    return exitCode;
}
