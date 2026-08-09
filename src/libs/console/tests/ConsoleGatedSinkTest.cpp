#include <gtest/gtest.h>

#include <antwika/console/fakes/FakeCountingSink.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/MouseButton.hpp>

#include "antwika/console/ConsoleGatedSink.hpp"

#include "antwika/console/ConsoleEvents.hpp"
#include "antwika/console/ConsoleState.hpp"
#include "antwika/console/InputFold.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::console::ConsoleGatedSink;
using antwika::console::consoleHeightAt;
using antwika::console::ConsoleState;
using antwika::console::InputFold;
using antwika::console::kConsoleAnimTicks;
using antwika::input::InputEventCodec;
using antwika::input::Key;
using antwika::input::KeyPressed;
using antwika::input::KeyReleased;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::input::PointerButtonReleased;
using antwika::input::PointerMoved;
using antwika::input::PointerScrolled;

namespace
{
    constexpr antwika::gfx::Size kTestCanvas{
        .width = 1024, .height = 640};

    constexpr auto kCanvas = kTestCanvas;

    struct Harness final
    {
        InputEventCodec codec;
        InputFold input{codec};
        ConsoleState console;
        antwika::console::fakes::FakeCountingSink inner;
        antwika::console::ConsoleEvents sent;
        ConsoleGatedSink gate{inner, console, input, sent};

        void openFully()
        {
            console.toggle();

            for (std::uint32_t step = 0; step < kConsoleAnimTicks;
                 ++step)
            {
                console.advance();
            }

            console.setHeight(
                consoleHeightAt(console.steps(), kCanvas));
        }

        void feed(const antwika::input::InputEvent &event)
        {
            const TickEvent ticked{
                .tick = 1, .event = codec.encode(event)};

            input.handle(ticked);
            gate.handle(ticked);
        }

        void feedTick()
        {
            const TickEvent ticked{
                .tick = 1,
                .event =
                    Event{.name = antwika::engine::events::kTick}};

            input.handle(ticked);
            gate.handle(ticked);
        }
    };
}

TEST(ConsoleGatedSinkTest, Handle_ForwardsEverythingWhileTheConsoleIsAway)
{
    Harness harness;

    harness.feed(KeyPressed{.key = Key::Space});
    harness.feed(PointerButtonPressed{
        .button = MouseButton::Left, .position = {.x = 10, .y = 10}});
    harness.feedTick();

    EXPECT_EQ(harness.inner.seen, 3);
}

TEST(ConsoleGatedSinkTest, Handle_AlwaysForwardsAnEngineTick)
{
    Harness harness;
    harness.openFully();

    harness.feedTick();

    EXPECT_EQ(harness.inner.seen, 1);
}

TEST(ConsoleGatedSinkTest, Handle_KeepsEveryKeyEdgeWhileTheConsoleIsOut)
{
    Harness harness;
    harness.openFully();

    harness.feed(KeyPressed{.key = Key::Space});
    harness.feed(KeyReleased{.key = Key::Space});

    EXPECT_EQ(harness.inner.seen, 0);
}

TEST(ConsoleGatedSinkTest, Handle_ClaimsAPressOnlyOverTheSheet)
{
    Harness harness;
    harness.openFully();

    harness.feed(PointerButtonPressed{
        .button = MouseButton::Left, .position = {.x = 10, .y = 10}});

    EXPECT_EQ(harness.inner.seen, 0);

    harness.feed(PointerButtonPressed{
        .button = MouseButton::Left,
        .position = {.x = 10, .y = 400}});

    EXPECT_EQ(harness.inner.seen, 1);
}

TEST(ConsoleGatedSinkTest, Handle_FollowsTheFoldedPointerForAScroll)
{
    Harness harness;
    harness.openFully();

    harness.feed(PointerMoved{.position = {.x = 10, .y = 10}});
    harness.feed(PointerScrolled{.vertical = 1});

    EXPECT_EQ(harness.inner.seen, 1);

    harness.feed(PointerMoved{.position = {.x = 10, .y = 400}});
    harness.feed(PointerScrolled{.vertical = 1});

    EXPECT_EQ(harness.inner.seen, 3);
}

TEST(ConsoleGatedSinkTest, Handle_PassesAReleaseOverTheSheet)
{
    Harness harness;
    harness.openFully();

    harness.feed(PointerButtonReleased{
        .button = MouseButton::Left, .position = {.x = 10, .y = 10}});

    EXPECT_EQ(harness.inner.seen, 1);
}
