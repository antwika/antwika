#include "antwika/life/Life.hpp"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <string_view>
#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/event/EventQueue.hpp>
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

#include "antwika/life/Events.hpp"
#include "antwika/life/PrintSystem.hpp"

using antwika::event::Event;
using antwika::event::EventQueue;
using antwika::event::EventRecorder;
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
    constexpr Tick kDemoTotalTicks = 4;

    // Stands in for real (network/keyboard) live input the engine lacks.
    // See blog/2026-07-27-building-a-deterministic-replay-system.md.
    // Seeds a horizontal blinker -- a period-2 oscillator -- at tick 0.
    std::vector<TimedEvent> demoScript()
    {
        return {
            TimedEvent{
                .tick = 0,
                .event = Event{
                    .name = antwika::life::events::kToggleCell,
                    .payload = "1,2",
                },
            },
            TimedEvent{
                .tick = 0,
                .event = Event{
                    .name = antwika::life::events::kToggleCell,
                    .payload = "2,2",
                },
            },
            TimedEvent{
                .tick = 0,
                .event = Event{
                    .name = antwika::life::events::kToggleCell,
                    .payload = "3,2",
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
    EventQueue eventQueue;
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
            eventQueue,
            eventSink,
            source,
            kDemoTotalTicks,
            kBoardWidth,
            kBoardHeight,
            {printSystem});
        return 0;
    }

    auto script = demoScript();
    ReplaySource source(script);
    antwika::life::bootstrap(
        clock,
        appender,
        formatter,
        logPolicy,
        eventQueue,
        eventSink,
        source,
        kDemoTotalTicks,
        kBoardWidth,
        kBoardHeight,
        {printSystem});

    if (!recordPath.empty())
    {
        std::ofstream replayFile(std::string(recordPath), std::ios::binary);
        BinaryReplayWriter writer(codec);
        writer.write(script, replayFile);
    }

    return 0;
}
