#include "antwika/game/Game.hpp"

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

#include <fstream>
#include <iostream>
#include <optional>
#include <string_view>
#include <vector>

#include "antwika/game/Events.hpp"

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
using antwika::time::SystemClock;
using antwika::time::Tick;

namespace
{
    // Stands in for real (network/keyboard) live input the engine lacks.
    // See blog/2026-07-27-building-a-deterministic-replay-system.md.
    // Ends with engine.stop -- the run keeps going until it is dispatched.
    std::vector<TimedEvent> demoScript()
    {
        return {
            TimedEvent{
                .tick = 1,
                .event = Event{
                    .name = antwika::game::events::kScoreIncrement,
                    .payload = R"({"amount":5})",
                },
            },
            TimedEvent{
                .tick = 3,
                .event = Event{
                    .name = antwika::game::events::kScoreIncrement,
                    .payload = R"({"amount":2})",
                },
            },
            TimedEvent{
                .tick = 4,
                .event = Event{.name = antwika::engine::events::kStop},
            },
        };
    }

    void printState(const antwika::game::GameState &state)
    {
        std::cout << "Final state: ticksProcessed=" << state.ticksProcessed
                   << " score=" << state.score << '\n';
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
                       || event.event.name == "Running Antwika Game";
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

    if (!replayPath.empty())
    {
        std::ifstream replayFile(std::string(replayPath), std::ios::binary);
        BinaryReplayReader reader(codec);
        auto loadedEvents = reader.read(replayFile);
        ReplaySource source(std::move(loadedEvents));
        auto state = antwika::game::bootstrap(
            clock,
            appender,
            formatter,
            logPolicy,
            eventSink,
            source);
        printState(state);
        return 0;
    }

    auto script = demoScript();
    ReplaySource source(script);
    ReplayRecorder replayRecorder;
    auto state = antwika::game::bootstrap(
        clock,
        appender,
        formatter,
        logPolicy,
        eventSink,
        source,
        std::nullopt,
        &replayRecorder);
    printState(state);

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
