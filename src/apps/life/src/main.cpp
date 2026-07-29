#include "antwika/life/Life.hpp"

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

#include "antwika/life/Events.hpp"
#include "antwika/life/PrintSystem.hpp"

using antwika::event::Event;
using antwika::event::EventRecorder;
using antwika::event::ReplayRecorder;
using antwika::event::TimedEvent;
using antwika::life::PrintSystem;
using antwika::log::Level;
using antwika::log::MinimumLevelLogPolicy;
using antwika::log::PlainFormatter;
using antwika::log::StreamAppender;
using antwika::replay::BinaryEventCodec;
using antwika::replay::BinaryReplayReader;
using antwika::replay::BinaryReplayWriter;
using antwika::replay::ReplaySource;
using antwika::time::SystemClock;
using antwika::time::Tick;

namespace
{
    constexpr std::uint32_t kBoardWidth = 5;
    constexpr std::uint32_t kBoardHeight = 5;

    // Stands in for real (network/keyboard) live input the engine lacks.
    // See blog/2026-07-27-building-a-deterministic-replay-system.md.
    // Seeds a horizontal blinker -- a period-2 oscillator -- at tick 0.
    // Ends with engine.stop after 4 generations.
    std::vector<TimedEvent> demoScript()
    {
        return {
            TimedEvent{
                .tick = 0,
                .event = Event{
                    .name = antwika::life::events::kToggleCell,
                    .payload = R"({"x":1,"y":2})",
                },
            },
            TimedEvent{
                .tick = 0,
                .event = Event{
                    .name = antwika::life::events::kToggleCell,
                    .payload = R"({"x":2,"y":2})",
                },
            },
            TimedEvent{
                .tick = 0,
                .event = Event{
                    .name = antwika::life::events::kToggleCell,
                    .payload = R"({"x":3,"y":2})",
                },
            },
            TimedEvent{
                .tick = 3,
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
                return event.event.name == antwika::engine::events::kTick
                       || event.event.name == "Running Antwika Life";
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
    PrintSystem printSystem(kBoardWidth, std::cout);

    if (!replayPath.empty())
    {
        std::ifstream replayFile(std::string(replayPath), std::ios::binary);
        BinaryReplayReader reader(codec);
        auto loadedEvents = reader.read(replayFile);
        ReplaySource source(std::move(loadedEvents));
        antwika::life::bootstrap(
            clock,
            appender,
            formatter,
            logPolicy,
            eventSink,
            source,
            kBoardWidth,
            kBoardHeight,
            {printSystem});
        return 0;
    }

    auto script = demoScript();
    ReplaySource source(script);
    ReplayRecorder replayRecorder;
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
