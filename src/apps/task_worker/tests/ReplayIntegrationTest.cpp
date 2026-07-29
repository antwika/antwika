#include <gtest/gtest.h>

#include <sstream>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/EventRecorder.hpp>
#include <antwika/event/TimedEvent.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/log/MinimumLevelLogPolicy.hpp>
#include <antwika/log/NullAppender.hpp>
#include <antwika/log/PlainFormatter.hpp>
#include <antwika/replay/BinaryEventCodec.hpp>
#include <antwika/replay/BinaryReplayReader.hpp>
#include <antwika/replay/BinaryReplayWriter.hpp>
#include <antwika/replay/IReplaySource.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/time/fakes/FakeClock.hpp>

#include "antwika/task_worker/Events.hpp"
#include "antwika/task_worker/TaskWorker.hpp"
#include "antwika/task_worker/Worker.hpp"

using antwika::event::Event;
using antwika::event::EventRecorder;
using antwika::event::TimedEvent;
using antwika::log::Level;
using antwika::log::MinimumLevelLogPolicy;
using antwika::log::NullAppender;
using antwika::log::PlainFormatter;
using antwika::replay::BinaryEventCodec;
using antwika::replay::BinaryReplayReader;
using antwika::replay::BinaryReplayWriter;
using antwika::replay::IReplaySource;
using antwika::replay::ReplaySource;
using antwika::task_worker::Worker;
using antwika::time::fakes::FakeClock;

namespace
{
    using antwika::task_worker::events::kTaskSubmit;
    constexpr antwika::time::Tick kMaxTicks = 10;
    constexpr std::uint32_t kWorkerCount = 2;

    std::vector<Worker> runTaskWorker(IReplaySource &source)
    {
        std::chrono::system_clock::time_point time{};
        FakeClock fakeClock(time);
        NullAppender appender;
        PlainFormatter formatter;
        MinimumLevelLogPolicy logPolicy(Level::Info);
        EventRecorder eventSink;

        return antwika::task_worker::bootstrap(
            fakeClock,
            appender,
            formatter,
            logPolicy,
            eventSink,
            source,
            kWorkerCount,
            {},
            nullptr,
            kMaxTicks);
    }
} // namespace

// This is the requirement this project exists for.
// Save a replay from a live run, then load it back.
// Prove the simulation reaches exactly the same worker state.
// Both runs go through the real bootstrap() entry point.
// That's the same entry point main.cpp uses, not a test-only shortcut.
TEST(ReplayIntegrationTest, LoadingASavedReplayReproducesTheSameState)
{
    std::vector<TimedEvent> script{
        TimedEvent{
            .tick = 0,
            .event = Event{
                .name = kTaskSubmit,
                .payload = R"({"id":1,"priority":1,)"
                           R"("durationTicks":4,"label":"Alpha"})"}},
        TimedEvent{
            .tick = 0,
            .event = Event{
                .name = kTaskSubmit,
                .payload = R"({"id":2,"priority":1,)"
                           R"("durationTicks":5,"label":"Beta"})"}},
        TimedEvent{
            .tick = 0,
            .event = Event{
                .name = kTaskSubmit,
                .payload = R"({"id":3,"priority":0,)"
                           R"("durationTicks":2,"label":"Gamma"})"}},
        TimedEvent{
            .tick = 4,
            .event = Event{
                .name = kTaskSubmit,
                .payload = R"({"id":4,"priority":3,)"
                           R"("durationTicks":1,"label":"Delta"})"}},
        TimedEvent{
            .tick = 4,
            .event = Event{
                .name = kTaskSubmit,
                .payload = R"({"id":5,"priority":1,)"
                           R"("durationTicks":1,"label":"Epsilon",)"
                           R"("dependsOnId":4})"}},
        TimedEvent{
            .tick = 5,
            .event = Event{.name = antwika::engine::events::kStop}},
    };

    ReplaySource liveSource(script);
    auto liveState = runTaskWorker(liveSource);

    BinaryEventCodec codec;
    BinaryReplayWriter writer(codec);
    std::stringstream replayStream;
    writer.write(script, replayStream);

    BinaryReplayReader reader(codec);
    auto loadedEvents = reader.read(replayStream);
    ReplaySource replaySource(loadedEvents);
    auto replayedState = runTaskWorker(replaySource);

    EXPECT_EQ(replayedState, liveState);
}
