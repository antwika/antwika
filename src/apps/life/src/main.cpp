#include "antwika/life/Life.hpp"

#include <array>
#include <cstdint>
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

#include "antwika/life/PrintSystem.hpp"

using antwika::event::EventRecorder;
using antwika::event::TickEventRecorder;
using antwika::life::PrintSystem;
using antwika::log::Level;
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
        "Running Antwika Life",
    };
} // namespace

int main(int argc, char **argv)
{
    const auto options = antwika::replay::parseReplayCliOptions(argc, argv);

    SystemClock clock;
    StreamAppender appender(std::cout);
    PlainFormatter formatter;
    MinimumLevelLogPolicy logPolicy(Level::Info);
    EventRecorder eventSink;
    TickEventRecorder replayRecorder;
    PrintSystem printSystem(kBoardWidth, std::cout);

    auto events = antwika::replay::loadReplayFile(
        options.replayPath.value_or(std::string(kDemoReplayPath)));
    ReplaySource source(std::move(events));

    antwika::life::bootstrap(
        clock,
        appender,
        formatter,
        logPolicy,
        eventSink,
        source,
        kBoardWidth,
        kBoardHeight,
        {printSystem},
        std::nullopt,
        &replayRecorder);

    if (options.recordPath)
    {
        antwika::replay::saveReplayFile(
            replayRecorder.getEvents(),
            *options.recordPath,
            kSelfGeneratedEventNames);
    }

    return 0;
}
