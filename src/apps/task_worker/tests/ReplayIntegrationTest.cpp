#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <sstream>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/mocks/MockEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/event/ITickEventSource.hpp>
#include <antwika/replay/ReplayReader.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/replay/ReplayWriter.hpp>

#include "antwika/task_worker/Events.hpp"
#include "antwika/task_worker/TaskWorker.hpp"
#include "antwika/task_worker/Worker.hpp"

using antwika::event::Event;
using antwika::event::mocks::MockEventSink;
using antwika::event::TickEvent;
using antwika::log::Level;
using antwika::event::ITickEventSource;
using antwika::replay::ReplayReader;
using antwika::replay::ReplaySource;
using antwika::replay::ReplayWriter;
using antwika::task_worker::Worker;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

namespace
{
    using antwika::task_worker::events::kTaskSubmit;
    constexpr antwika::time::Tick kMaxTicks = 10;
    constexpr std::uint32_t kWorkerCount = 2;

    std::vector<Worker> runTaskWorker(ITickEventSource &source)
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> eventSink;

        return antwika::task_worker::bootstrap(
            antwika::task_worker::TaskWorkerWiring{
                .logger = logger,
                .eventSink = eventSink,
                .inputSource = source,
                .workerCount = kWorkerCount,
                .maxTicks = kMaxTicks});
    }
} // namespace

// This is the requirement this project exists for.
// Save a replay from a live run, then load it back.
// Prove the simulation reaches exactly the same worker state.
// Both runs go through the real bootstrap() entry point.
// That's the same entry point main.cpp uses, not a test-only shortcut.
TEST(ReplayIntegrationTest, LoadingASavedReplayReproducesTheSameState)
{
    std::vector<TickEvent> script{
        TickEvent{
            .tick = 0,
            .event = Event{
                .name = kTaskSubmit,
                .payload = R"({"id":1,"priority":1,)"
                           R"("durationTicks":4,"label":"Alpha"})"}},
        TickEvent{
            .tick = 0,
            .event = Event{
                .name = kTaskSubmit,
                .payload = R"({"id":2,"priority":1,)"
                           R"("durationTicks":5,"label":"Beta"})"}},
        TickEvent{
            .tick = 0,
            .event = Event{
                .name = kTaskSubmit,
                .payload = R"({"id":3,"priority":0,)"
                           R"("durationTicks":2,"label":"Gamma"})"}},
        TickEvent{
            .tick = 4,
            .event = Event{
                .name = kTaskSubmit,
                .payload = R"({"id":4,"priority":3,)"
                           R"("durationTicks":1,"label":"Delta"})"}},
        TickEvent{
            .tick = 4,
            .event = Event{
                .name = kTaskSubmit,
                .payload = R"({"id":5,"priority":1,)"
                           R"("durationTicks":1,"label":"Epsilon",)"
                           R"("dependsOnId":4})"}},
        TickEvent{
            .tick = 5,
            .event = Event{.name = antwika::engine::events::kStop}},
    };

    ReplaySource liveSource(script);
    auto liveState = runTaskWorker(liveSource);

    ReplayWriter writer;
    std::stringstream replayStream;
    writer.write(script, replayStream);

    ReplayReader reader;
    auto loadedEvents = reader.read(replayStream);
    ReplaySource replaySource(loadedEvents);
    auto replayedState = runTaskWorker(replaySource);

    EXPECT_EQ(replayedState, liveState);
}
