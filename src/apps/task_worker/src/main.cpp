#include "antwika/task_worker/TaskWorker.hpp"

#include <cstdint>
#include <cstdlib>
#include <exception>
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

#include "antwika/task_worker/StatusPrintSystem.hpp"
#include "antwika/task_worker/TaskRegistry.hpp"

using antwika::event::EventRecorder;
using antwika::event::TickEventRecorder;
using antwika::log::Level;
using antwika::log::MinimumLevelLogPolicy;
using antwika::log::PlainFormatter;
using antwika::log::StreamAppender;
using antwika::replay::ReplaySource;
using antwika::task_worker::StatusPrintSystem;
using antwika::task_worker::TaskRegistry;
using antwika::time::SystemClock;

namespace
{
    constexpr std::uint32_t kWorkerCount = 2;

    constexpr std::string_view kDemoReplayPath =
        ANTWIKA_TASK_WORKER_DEMO_REPLAY_PATH;

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
    TaskRegistry registry;
    StatusPrintSystem printSystem(std::cout, registry);

    // Catching is what makes the run's resources unwind at all.
    // An uncaught exception may call std::terminate without unwinding.
    // Catching here also lets a failed --record run save what it has.
    int exitCode = EXIT_SUCCESS;
    try
    {
        auto events = antwika::replay::loadReplayFile(
            options.replayPath.value_or(std::string(kDemoReplayPath)));
        ReplaySource source(std::move(events));

        antwika::task_worker::bootstrap(
            clock,
            appender,
            formatter,
            logPolicy,
            eventSink,
            source,
            kWorkerCount,
            {printSystem},
            &registry,
            std::nullopt,
            &replayRecorder);
    }
    catch (const std::exception &error)
    {
        std::cerr << "antwika_task_worker: " << error.what() << '\n';
        exitCode = EXIT_FAILURE;
    }

    if (options.recordPath)
    {
        antwika::replay::saveReplayFile(
            replayRecorder.getEvents(), *options.recordPath);
    }

    return exitCode;
}
