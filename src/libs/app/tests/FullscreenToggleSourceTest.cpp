#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <utility>
#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/mocks/MockWindow.hpp>
#include <antwika/input/InputError.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/app/FullscreenToggleSource.hpp"

using antwika::app::FullscreenToggleSource;
using antwika::event::Event;
using antwika::event::EventName;
using antwika::event::TickEvent;
using antwika::gfx::mocks::MockWindow;
using antwika::input::InputEventCodec;
using antwika::input::Key;
using antwika::input::KeyPressed;
using antwika::input::KeyReleased;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::replay::ReplaySource;
using antwika::time::Tick;
using ::testing::NiceMock;
using ::testing::Return;

namespace
{
    const InputEventCodec kCodec;

    [[nodiscard]] TickEvent getEntryAt(Tick tick, Event event)
    {
        return TickEvent{.tick = tick, .event = std::move(event)};
    }

    class FullscreenState final
    {
    public:
        explicit FullscreenState(MockWindow &window)
        {
            ON_CALL(window, isFullscreen())
                .WillByDefault([this] { return filling; });
            ON_CALL(window, setFullscreen(::testing::_))
                .WillByDefault([this](bool wantedFullscreen)
                               { filling = wantedFullscreen; });
        }

        [[nodiscard]] bool isFilled() const noexcept
        {
            return filling;
        }

    private:
        bool filling = false;
    };
}

TEST(FullscreenToggleSourceTest, EventsFor_FillsTheScreenOnTheKey)
{
    ReplaySource innerSource(
        {getEntryAt(0, kCodec.getEncodedEvent(KeyPressed{.key = Key::F10}))});
    NiceMock<MockWindow> window;
    const FullscreenState state(window);

    FullscreenToggleSource source(innerSource, window, kCodec, Key::F10);

    const auto events = source.eventsFor(0);

    EXPECT_TRUE(state.isFilled());

    EXPECT_EQ(
        events,
        (std::vector<Event>{kCodec.getEncodedEvent(KeyPressed{.key = Key::F10})}));
}

TEST(FullscreenToggleSourceTest, EventsFor_PutsTheWindowBackOnTheNextPress)
{
    ReplaySource innerSource(
        {getEntryAt(0, kCodec.getEncodedEvent(KeyPressed{.key = Key::F10})),
         getEntryAt(1, kCodec.getEncodedEvent(KeyPressed{.key = Key::F10}))});

    NiceMock<MockWindow> window;
    const FullscreenState state(window);

    FullscreenToggleSource source(innerSource, window, kCodec, Key::F10);

    EXPECT_EQ(source.eventsFor(0).size(), 1U);
    EXPECT_TRUE(state.isFilled());

    EXPECT_EQ(source.eventsFor(1).size(), 1U);
    EXPECT_FALSE(state.isFilled());
}

TEST(FullscreenToggleSourceTest, EventsFor_IgnoresARepeat)
{
    ReplaySource innerSource(
        {getEntryAt(0,
            kCodec.getEncodedEvent(
                KeyPressed{.key = Key::F10, .repeat = true}))});

    NiceMock<MockWindow> window;

    EXPECT_CALL(window, setFullscreen(::testing::_)).Times(0);

    FullscreenToggleSource source(innerSource, window, kCodec, Key::F10);

    EXPECT_EQ(source.eventsFor(0).size(), 1U);
}

TEST(FullscreenToggleSourceTest, EventsFor_IgnoresAnotherKey)
{
    ReplaySource innerSource(
        {getEntryAt(0, kCodec.getEncodedEvent(KeyPressed{.key = Key::F11})),
         getEntryAt(0, kCodec.getEncodedEvent(KeyReleased{.key = Key::F10}))});

    NiceMock<MockWindow> window;

    EXPECT_CALL(window, setFullscreen(::testing::_)).Times(0);

    FullscreenToggleSource source(innerSource, window, kCodec, Key::F10);

    EXPECT_EQ(source.eventsFor(0).size(), 2U);
}

TEST(FullscreenToggleSourceTest, EventsFor_IgnoresAnEventThatIsNotInput)
{
    ReplaySource innerSource(
        {getEntryAt(0, Event{.name = EventName{"game.score_increment"}, .payload = "{}"}),
         getEntryAt(0,
            kCodec.getEncodedEvent(
                PointerButtonPressed{.button = MouseButton::Left}))});

    NiceMock<MockWindow> window;

    EXPECT_CALL(window, setFullscreen(::testing::_)).Times(0);

    FullscreenToggleSource source(innerSource, window, kCodec, Key::F10);

    EXPECT_EQ(source.eventsFor(0).size(), 2U);
}

TEST(FullscreenToggleSourceTest, EventsFor_LetsABadPayloadThrough)
{
    ReplaySource innerSource(
        {getEntryAt(0,
            Event{
                .name = EventName{"input.key_down"},
                .payload = R"({"key":"NotAKey"})"})});

    NiceMock<MockWindow> window;

    EXPECT_CALL(window, setFullscreen(::testing::_)).Times(0);

    FullscreenToggleSource source(innerSource, window, kCodec, Key::F10);

    EXPECT_THROW(
        {
            const auto events = source.eventsFor(0);
        },
        antwika::input::InputError);
}
