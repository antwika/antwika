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

#include "DemoScript.hpp"
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
using antwika::task_worker::tests::demoScript;
using antwika::task_worker::tests::kMaxTicks;
using antwika::task_worker::tests::kWorkerCount;
using ::testing::NiceMock;

namespace
{
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
                .maxTicks = kMaxTicks}).workers;
    }
}

TEST(ReplayIntegrationTest, Replay_ReproducesTheSameState)
{
    auto script = demoScript();

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
