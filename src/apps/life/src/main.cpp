#include "antwika/life/Life.hpp"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>
#include <string_view>

#include <antwika/event/EventRecorder.hpp>
#include <antwika/event/TickEventRecorder.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/log/Logger.hpp>
#include <antwika/log/MinimumLevelLogPolicy.hpp>
#include <antwika/log/PlainFormatter.hpp>
#include <antwika/log/StreamAppender.hpp>
#include <antwika/replay/ReplayCli.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/time/SystemClock.hpp>

#include "antwika/life/Events.hpp"
#include "antwika/life/PrintSystem.hpp"

using antwika::event::EventRecorder;
using antwika::event::TickEventRecorder;
using antwika::life::PrintSystem;
using antwika::log::Level;
using antwika::log::Logger;
using antwika::log::MinimumLevelLogPolicy;
using antwika::log::PlainFormatter;
using antwika::log::StreamAppender;
using antwika::replay::ReplaySource;
using antwika::time::SystemClock;

namespace
{
    constexpr std::uint32_t kBoardWidth = 5;
    constexpr std::uint32_t kBoardHeight = 5;

    constexpr std::string_view kDemoReplayPath = ANTWIKA_LIFE_DEMO_REPLAY_PATH;

    constexpr std::array<std::string_view, 1> kSelfGeneratedEventNames{
        antwika::life::events::kStarted,
    };
} // namespace

int main(int argc, char **argv)
{
    const auto options = antwika::replay::parseReplayCliOptions(argc, argv);

    SystemClock clock;
    StreamAppender appender(std::cout);
    PlainFormatter formatter;
    MinimumLevelLogPolicy logPolicy(Level::Info);
    Logger logger(formatter, logPolicy, clock, appender);
    EventRecorder eventSink;
    TickEventRecorder replayRecorder;
    PrintSystem printSystem(kBoardWidth, std::cout);

    // Catching is what makes the run's resources unwind at all.
    // An uncaught exception may call std::terminate without unwinding.
    // Catching here also lets a failed --record run save what it has.
    int exitCode = EXIT_SUCCESS;
    try
    {
        auto events = antwika::replay::loadReplayFile(
            options.replayPath.value_or(std::string(kDemoReplayPath)));
        ReplaySource source(std::move(events));

        antwika::life::bootstrap(
            logger,
            eventSink,
            source,
            kBoardWidth,
            kBoardHeight,
            {printSystem},
            std::nullopt,
            &replayRecorder);
    }
    catch (const std::exception &error)
    {
        std::cerr << "antwika_life: " << error.what() << '\n';
        exitCode = EXIT_FAILURE;
    }

    if (options.recordPath)
    {
        antwika::replay::saveReplayFile(
            replayRecorder.getEvents(),
            *options.recordPath,
            kSelfGeneratedEventNames);
    }

    return exitCode;
}
