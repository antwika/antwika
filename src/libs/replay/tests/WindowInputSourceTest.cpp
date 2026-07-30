#include "antwika/replay/WindowInputSource.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <optional>
#include <vector>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/WindowEvent.hpp>
#include <antwika/gfx/WindowId.hpp>
#include <antwika/gfx/mocks/MockGfxBackend.hpp>
#include <antwika/replay/ReplaySource.hpp>

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::gfx::CloseRequested;
using antwika::gfx::Resized;
using antwika::gfx::WindowEvent;
using antwika::gfx::WindowId;
using antwika::gfx::mocks::MockGfxBackend;
using antwika::replay::ReplaySource;
using antwika::replay::WindowInputSource;
using ::testing::NiceMock;
using ::testing::Return;

namespace
{
    constexpr WindowId kOurWindow{7};
    constexpr WindowId kSomeoneElsesWindow{8};

    // Anything an application might have put in the tick stream.
    const Event kScripted{
        .name = "app.something",
        .payload = R"({"x":1,"y":2})",
    };

    const Event kStop{.name = antwika::engine::events::kStop};

    std::optional<WindowEvent> closeOf(WindowId window)
    {
        return WindowEvent{.window = window, .payload = CloseRequested{}};
    }
} // namespace

TEST(WindowInputSourceTest, EventsFor_ForwardsTheInnerSourcesEventsUnchanged)
{
    ReplaySource inner({TickEvent{.tick = 0, .event = kScripted}});
    NiceMock<MockGfxBackend> backend;
    EXPECT_CALL(backend, pollEvent()).WillOnce(Return(std::nullopt));

    WindowInputSource source(inner, backend, kOurWindow);

    EXPECT_EQ(source.eventsFor(0), (std::vector<Event>{kScripted}));
}

TEST(
    WindowInputSourceTest,
    EventsFor_AppendsAnEngineStopAfterTheInnerEventsOnACloseRequest)
{
    ReplaySource inner({TickEvent{.tick = 0, .event = kScripted}});
    NiceMock<MockGfxBackend> backend;
    EXPECT_CALL(backend, pollEvent())
        .WillOnce(Return(closeOf(kOurWindow)))
        .WillOnce(Return(std::nullopt));

    WindowInputSource source(inner, backend, kOurWindow);

    EXPECT_EQ(source.eventsFor(0), (std::vector<Event>{kScripted, kStop}));
}

TEST(
    WindowInputSourceTest,
    EventsFor_IgnoresACloseRequestForSomebodyElsesWindow)
{
    ReplaySource inner({});
    NiceMock<MockGfxBackend> backend;
    EXPECT_CALL(backend, pollEvent())
        .WillOnce(Return(closeOf(kSomeoneElsesWindow)))
        .WillOnce(Return(std::nullopt));

    WindowInputSource source(inner, backend, kOurWindow);

    EXPECT_TRUE(source.eventsFor(0).empty());
}

TEST(WindowInputSourceTest, EventsFor_IgnoresEventsThatAreNotCloseRequests)
{
    ReplaySource inner({});
    NiceMock<MockGfxBackend> backend;
    EXPECT_CALL(backend, pollEvent())
        .WillOnce(
            Return(WindowEvent{
                .window = kOurWindow,
                .payload = Resized{.size = {.width = 10, .height = 10}}}))
        .WillOnce(Return(std::nullopt));

    WindowInputSource source(inner, backend, kOurWindow);

    EXPECT_TRUE(source.eventsFor(0).empty());
}

// A window system is free to report the same request more than once.
// The run still only stops once.
TEST(
    WindowInputSourceTest,
    EventsFor_AppendsOneEngineStopForRepeatedCloseRequests)
{
    ReplaySource inner({});
    NiceMock<MockGfxBackend> backend;
    EXPECT_CALL(backend, pollEvent())
        .WillOnce(Return(closeOf(kOurWindow)))
        .WillOnce(Return(closeOf(kOurWindow)))
        .WillOnce(Return(std::nullopt));

    WindowInputSource source(inner, backend, kOurWindow);

    EXPECT_EQ(source.eventsFor(0), (std::vector<Event>{kStop}));
}

// The queue serves every window, so it has to be emptied either way.
TEST(WindowInputSourceTest, EventsFor_DrainsThePendingQueueEveryTick)
{
    ReplaySource inner({});
    NiceMock<MockGfxBackend> backend;
    EXPECT_CALL(backend, pollEvent())
        .WillOnce(Return(closeOf(kSomeoneElsesWindow)))
        .WillOnce(Return(closeOf(kSomeoneElsesWindow)))
        .WillOnce(Return(std::nullopt))
        .WillOnce(Return(std::nullopt));

    WindowInputSource source(inner, backend, kOurWindow);

    EXPECT_TRUE(source.eventsFor(0).empty());
    EXPECT_TRUE(source.eventsFor(1).empty());
}
