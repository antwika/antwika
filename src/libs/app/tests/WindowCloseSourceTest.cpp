#include "antwika/app/WindowCloseSource.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <optional>
#include <vector>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/WindowEvent.hpp>
#include <antwika/gfx/WindowId.hpp>
#include <antwika/gfx/mocks/MockGfxBackend.hpp>
#include <antwika/gfx/mocks/MockWindow.hpp>
#include <antwika/replay/ReplaySource.hpp>

using antwika::app::WindowCloseSource;
using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::gfx::CloseRequested;
using antwika::gfx::Resized;
using antwika::gfx::Size;
using antwika::gfx::WindowEvent;
using antwika::gfx::WindowId;
using antwika::gfx::mocks::MockGfxBackend;
using antwika::gfx::mocks::MockWindow;
using antwika::replay::ReplaySource;
using ::testing::NiceMock;
using ::testing::Return;

namespace
{
    constexpr WindowId kOurWindow{1};
    constexpr WindowId kSomeoneElsesWindow{99};

    // Anything an application might have put in the tick stream.
    constexpr const char *kScripted = "app.something";

    [[nodiscard]] std::vector<TickEvent> oneEventAtTickZero()
    {
        return {
            TickEvent{
                .tick = 0,
                .event =
                    Event{.name = kScripted, .payload = R"({"x":1})"}}};
    }

    [[nodiscard]] WindowEvent closeOf(WindowId id)
    {
        return WindowEvent{.window = id, .payload = CloseRequested{}};
    }
} // namespace

class WindowCloseSourceTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ON_CALL(window, id()).WillByDefault(Return(kOurWindow));
        ON_CALL(window, isOpen()).WillByDefault(Return(true));
    }

    // A window that really closes.
    // Three collaborators read isOpen() within one tick.
    // A call-count script would say nothing about what each of them saw.
    void closeOnRequest()
    {
        ON_CALL(window, isOpen()).WillByDefault([this] { return open; });
        ON_CALL(window, close()).WillByDefault([this] { open = false; });
    }

    bool open = true;
    NiceMock<MockGfxBackend> backend;
    NiceMock<MockWindow> window;
    ReplaySource inner{oneEventAtTickZero()};
};

TEST_F(WindowCloseSourceTest, EventsFor_PassesTheWrappedEventsThrough)
{
    EXPECT_CALL(backend, pollEvent()).WillOnce(Return(std::nullopt));

    WindowCloseSource source(inner, backend, window);

    const auto events = source.eventsFor(0);

    ASSERT_EQ(events.size(), 1);
    EXPECT_EQ(events.at(0).name, kScripted);
}

TEST_F(WindowCloseSourceTest, EventsFor_AddsNothingWhileTheWindowIsOpen)
{
    EXPECT_CALL(backend, pollEvent()).WillOnce(Return(std::nullopt));

    WindowCloseSource source(inner, backend, window);

    EXPECT_EQ(source.eventsFor(1).size(), 0);
}

TEST_F(WindowCloseSourceTest, EventsFor_StopsTheEngineOnACloseRequest)
{
    closeOnRequest();
    EXPECT_CALL(backend, pollEvent())
        .WillOnce(Return(closeOf(kOurWindow)))
        .WillOnce(Return(std::nullopt));
    EXPECT_CALL(window, close());

    WindowCloseSource source(inner, backend, window);

    const auto events = source.eventsFor(1);

    ASSERT_EQ(events.size(), 1);
    EXPECT_EQ(events.at(0).name, antwika::engine::events::kStop);
}

TEST_F(WindowCloseSourceTest, EventsFor_StopsTheEngineAfterTheTicksEvents)
{
    closeOnRequest();
    EXPECT_CALL(backend, pollEvent())
        .WillOnce(Return(closeOf(kOurWindow)))
        .WillOnce(Return(std::nullopt));

    WindowCloseSource source(inner, backend, window);

    const auto events = source.eventsFor(0);

    ASSERT_EQ(events.size(), 2);
    EXPECT_EQ(events.at(0).name, kScripted);
    EXPECT_EQ(events.at(1).name, antwika::engine::events::kStop);
}

TEST_F(WindowCloseSourceTest, EventsFor_IgnoresACloseForAnotherWindow)
{
    closeOnRequest();
    EXPECT_CALL(backend, pollEvent())
        .WillOnce(Return(closeOf(kSomeoneElsesWindow)))
        .WillOnce(Return(std::nullopt));
    EXPECT_CALL(window, close()).Times(0);

    WindowCloseSource source(inner, backend, window);

    EXPECT_EQ(source.eventsFor(1).size(), 0);
}

TEST_F(WindowCloseSourceTest, EventsFor_IgnoresAResize)
{
    closeOnRequest();
    EXPECT_CALL(backend, pollEvent())
        .WillOnce(
            Return(WindowEvent{
                .window = kOurWindow,
                .payload = Resized{.size = {.width = 640, .height = 480}}}))
        .WillOnce(Return(std::nullopt));
    EXPECT_CALL(window, close()).Times(0);

    WindowCloseSource source(inner, backend, window);

    EXPECT_EQ(source.eventsFor(1).size(), 0);
}

TEST_F(WindowCloseSourceTest, EventsFor_DrainsEveryPendingEvent)
{
    closeOnRequest();
    EXPECT_CALL(backend, pollEvent())
        .WillOnce(Return(closeOf(kSomeoneElsesWindow)))
        .WillOnce(
            Return(WindowEvent{
                .window = kOurWindow,
                .payload = Resized{.size = {.width = 640, .height = 480}}}))
        .WillOnce(Return(closeOf(kOurWindow)))
        .WillOnce(Return(std::nullopt));

    WindowCloseSource source(inner, backend, window);

    const auto events = source.eventsFor(1);

    ASSERT_EQ(events.size(), 1);
    EXPECT_EQ(events.at(0).name, antwika::engine::events::kStop);
}

TEST_F(WindowCloseSourceTest, EventsFor_StopsTheEngineForAnAlreadyShutWindow)
{
    ON_CALL(window, isOpen()).WillByDefault(Return(false));
    EXPECT_CALL(backend, pollEvent()).WillOnce(Return(std::nullopt));

    WindowCloseSource source(inner, backend, window);

    const auto events = source.eventsFor(1);

    ASSERT_EQ(events.size(), 1);
    EXPECT_EQ(events.at(0).name, antwika::engine::events::kStop);
}

TEST_F(WindowCloseSourceTest, EventsFor_KeepsSayingStopOnEveryLaterTick)
{
    // Deliberate.
    // StopSignal ends the loop on the tick carrying the first stop.
    // So a latch's taken branch would be unreachable.
    ON_CALL(window, isOpen()).WillByDefault(Return(false));
    EXPECT_CALL(backend, pollEvent())
        .WillOnce(Return(std::nullopt))
        .WillOnce(Return(std::nullopt));

    WindowCloseSource source(inner, backend, window);

    EXPECT_EQ(source.eventsFor(1).size(), 1);
    EXPECT_EQ(source.eventsFor(2).size(), 1);
}

TEST_F(WindowCloseSourceTest, PumpEvents_ClosesTheWindowOnItsOwn)
{
    closeOnRequest();
    EXPECT_CALL(backend, pollEvent())
        .WillOnce(Return(closeOf(kOurWindow)))
        .WillOnce(Return(std::nullopt));

    WindowCloseSource source(inner, backend, window);

    source.pumpEvents();

    EXPECT_FALSE(window.isOpen());
}
