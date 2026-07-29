#include "antwika/task_worker/TaskWorker.hpp"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <optional>
#include <string_view>
#include <vector>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/EventRecorder.hpp>
#include <antwika/event/ReplayRecorder.hpp>
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
#include "antwika/task_worker/StatusPrintSystem.hpp"
#include "antwika/task_worker/TaskRegistry.hpp"

using antwika::event::Event;
using antwika::event::EventRecorder;
using antwika::event::ReplayRecorder;
using antwika::event::TimedEvent;
using antwika::log::Level;
using antwika::log::MinimumLevelLogPolicy;
using antwika::log::PlainFormatter;
using antwika::log::StreamAppender;
using antwika::replay::BinaryEventCodec;
using antwika::replay::BinaryReplayReader;
using antwika::replay::BinaryReplayWriter;
using antwika::replay::ReplaySource;
using antwika::task_worker::StatusPrintSystem;
using antwika::task_worker::TaskRegistry;
using antwika::time::SystemClock;
using antwika::time::Tick;

namespace
{
    constexpr std::uint32_t kWorkerCount = 2;

    // Stands in for real (network/CLI) live input the engine lacks.
    // See blog/003-... and blog/004-... for the pattern this follows.
    // Sized to exercise multi-tick distribution and a priority jump.
    // Also exercises a dependency edge crossing a tick boundary.
    // See blog/006-... for the full scenario rationale.
    // Ends with engine.stop 3 ticks after every task has settled.
    // That gives the demo's printed status a few idle ticks to show.
    std::vector<TimedEvent> demoScript()
    {
        using antwika::task_worker::events::kTaskSubmit;

        return {
            TimedEvent{
                .tick = 0,
                .event = Event{
                    .name = kTaskSubmit,
                    .payload = R"({"id":1,"priority":1,)"
                               R"("durationTicks":4,"label":"Alpha"})",
                },
            },
            TimedEvent{
                .tick = 0,
                .event = Event{
                    .name = kTaskSubmit,
                    .payload = R"({"id":2,"priority":1,)"
                               R"("durationTicks":5,"label":"Beta"})",
                },
            },
            TimedEvent{
                .tick = 0,
                .event = Event{
                    .name = kTaskSubmit,
                    .payload = R"({"id":3,"priority":0,)"
                               R"("durationTicks":2,"label":"Gamma"})",
                },
            },
            TimedEvent{
                .tick = 4,
                .event = Event{
                    .name = kTaskSubmit,
                    .payload = R"({"id":4,"priority":3,)"
                               R"("durationTicks":1,"label":"Delta"})",
                },
            },
            TimedEvent{
                .tick = 4,
                .event = Event{
                    .name = kTaskSubmit,
                    .payload = R"({"id":5,"priority":1,)"
                               R"("durationTicks":1,"label":"Epsilon",)"
                               R"("dependsOnId":4})",
                },
            },
            TimedEvent{
                .tick = 7,
                .event = Event{.name = antwika::engine::events::kStop},
            },
        };
    }

    // engine.tick and the startup announcement are both self-generated.
    // Every bootstrap() call regenerates them fresh, live or replayed.
    // Recording either and feeding it back would double-dispatch it.
    // See blog/2026-07-27-building-a-deterministic-replay-system.md.
    std::vector<TimedEvent> stripSelfGeneratedEvents(
        std::vector<TimedEvent> events)
    {
        std::erase_if(
            events,
            [](const TimedEvent &event)
            {
                const auto &name = event.event.name;
                return name == antwika::engine::events::kTick
                       || name == "Running Antwika TaskWorker";
            });
        return events;
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
    TaskRegistry registry;
    StatusPrintSystem printSystem(std::cout, registry);

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
            kWorkerCount,
            {printSystem},
            &registry);
        return 0;
    }

    auto script = demoScript();
    ReplaySource source(script);
    ReplayRecorder replayRecorder;
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

    if (!recordPath.empty())
    {
        std::ofstream replayFile(std::string(recordPath), std::ios::binary);
        BinaryReplayWriter writer(codec);
        writer.write(
            stripSelfGeneratedEvents(replayRecorder.getEvents()),
            replayFile);
    }

    return 0;
}
