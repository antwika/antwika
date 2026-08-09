#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <sstream>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/mocks/MockEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/event/ITickEventSource.hpp>
#include <antwika/replay/ReplayReader.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/replay/ReplayWriter.hpp>

#include "BlinkerScript.hpp"
#include "antwika/life/Events.hpp"
#include "antwika/life/Life.hpp"

using antwika::event::Event;
using antwika::event::mocks::MockEventSink;
using antwika::event::TickEvent;
using antwika::life::Board;
using antwika::log::mocks::MockLogger;
using antwika::event::ITickEventSource;
using antwika::replay::ReplayReader;
using antwika::replay::ReplaySource;
using antwika::replay::ReplayWriter;
using antwika::life::tests::blinkerScript;
using ::testing::NiceMock;

namespace
{
    constexpr antwika::time::Tick kMaxTicks = 10;
    constexpr std::uint32_t kWidth = 5;
    constexpr std::uint32_t kHeight = 5;

    antwika::life::LifeSummary runLife(ITickEventSource &source)
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> eventSink;

        return antwika::life::bootstrap(
            antwika::life::LifeWiring{
                .logger = logger,
                .eventSink = eventSink,
                .inputSource = source,
                .width = kWidth,
                .height = kHeight,
                .maxTicks = kMaxTicks});
    }
}

TEST(ReplayIntegrationTest, Replay_ReproducesTheSameBoard)
{
    std::vector<TickEvent> script = blinkerScript();

    ReplaySource liveSource(script);
    auto liveBoard = runLife(liveSource);

    ReplayWriter writer;
    std::stringstream replayStream;
    writer.write(script, replayStream);

    ReplayReader reader;
    auto loadedEvents = reader.read(replayStream);
    ReplaySource replaySource(loadedEvents);
    auto replayedBoard = runLife(replaySource);

    EXPECT_EQ(replayedBoard, liveBoard);
}
