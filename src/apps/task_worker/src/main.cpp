#include "antwika/task_worker/TaskWorker.hpp"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <string_view>
#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/event/EventRecorder.hpp>
#include <antwika/event/TimedEvent.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/log/MinimumLevelLogPolicy.hpp>
#include <antwika/log/PlainFormatter.hpp>
#include <antwika/log/StreamAppender.hpp>
#include <antwika/replay/BinaryEventCodec.hpp>
#include <antwika/replay/BinaryReplayReader.hpp>
#include <antwika/replay/BinaryReplayWriter.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/time/SystemClock.hpp>

#include "antwika/task_worker/Events.hpp"
#include "antwika/task_worker/WorkerStatusPrintSystem.hpp"

using antwika::event::Event;
using antwika::event::EventRecorder;
using antwika::event::TimedEvent;
using antwika::log::Level;
using antwika::log::MinimumLevelLogPolicy;
using antwika::log::PlainFormatter;
using antwika::log::StreamAppender;
using antwika::replay::BinaryEventCodec;
using antwika::replay::BinaryReplayReader;
using antwika::replay::BinaryReplayWriter;
using antwika::replay::ReplaySource;
using antwika::task_worker::WorkerStatusPrintSystem;
using antwika::time::SystemClock;
using antwika::time::Tick;

namespace
{
    constexpr std::uint32_t kWorkerCount = 2;
    constexpr Tick kDemoTotalTicks = 6;

    // Stands in for real (network/CLI) live input the engine lacks.
    // See blog/003-... and blog/004-... for the pattern this follows.
    // Sized to exercise multi-tick distribution and a priority jump.
    // Also exercises a dependency edge crossing a tick boundary.
    // See PLAN_SCHEDULER.md §4.7 for the full scenario rationale.
    std::vector<TimedEvent> demoScript()
    {
        using antwika::task_worker::events::kTaskSubmit;

        return {
            TimedEvent{
                .tick = 0,
                .event = Event{
                    .name = kTaskSubmit,
                    .payload = "1,1,4,Alpha",
                },
            },
            TimedEvent{
                .tick = 0,
                .event = Event{
                    .name = kTaskSubmit,
                    .payload = "2,1,5,Beta",
                },
            },
            TimedEvent{
                .tick = 0,
                .event = Event{
                    .name = kTaskSubmit,
                    .payload = "3,0,1,Gamma",
                },
            },
            TimedEvent{
                .tick = 4,
                .event = Event{
                    .name = kTaskSubmit,
                    .payload = "4,3,1,Delta",
                },
            },
            TimedEvent{
                .tick = 4,
                .event = Event{
                    .name = kTaskSubmit,
                    .payload = "5,1,1,Epsilon,4",
                },
            },
        };
    }
} // namespace

int main(int argc, char **argv)
{
    std::string_view recordPath;
    std::string_view replayPath;
    for (int i = 1; i < argc; ++i)
    {
        const std::string_view arg = argv[i];
        if (arg == "--record" && i + 1 < argc)
        {
            recordPath = argv[++i];
        }
        else if (arg == "--replay" && i + 1 < argc)
        {
            replayPath = argv[++i];
        }
    }

    SystemClock clock;
    StreamAppender appender(std::cout);
    PlainFormatter formatter;
    MinimumLevelLogPolicy logPolicy(Level::Info);
    EventRecorder eventSink;
    BinaryEventCodec codec;
    WorkerStatusPrintSystem printSystem(std::cout);

    if (!replayPath.empty())
    {
        std::ifstream replayFile(std::string(replayPath), std::ios::binary);
        BinaryReplayReader reader(codec);
        auto loadedEvents = reader.read(replayFile);
        ReplaySource source(std::move(loadedEvents));
        antwika::task_worker::bootstrap(
            clock,
            appender,
            formatter,
            logPolicy,
            eventSink,
            source,
            kDemoTotalTicks,
            kWorkerCount,
            {printSystem});
        return 0;
    }

    auto script = demoScript();
    ReplaySource source(script);
    antwika::task_worker::bootstrap(
        clock,
        appender,
        formatter,
        logPolicy,
        eventSink,
        source,
        kDemoTotalTicks,
        kWorkerCount,
        {printSystem});

    if (!recordPath.empty())
    {
        std::ofstream replayFile(std::string(recordPath), std::ios::binary);
        BinaryReplayWriter writer(codec);
        writer.write(script, replayFile);
    }

    return 0;
}
