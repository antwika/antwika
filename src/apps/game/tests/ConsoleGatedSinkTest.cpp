#include <gtest/gtest.h>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/MouseButton.hpp>

#include "antwika/game/ConsoleGatedSink.hpp"
#include "antwika/game/ConsoleState.hpp"
#include "antwika/game/InputFold.hpp"
#include "antwika/game/UiCanvas.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::game::ConsoleGatedSink;
using antwika::game::consoleHeightAt;
using antwika::game::ConsoleState;
using antwika::game::InputFold;
using antwika::game::kConsoleAnimTicks;
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
    constexpr auto kCanvas = antwika::game::kUiCanvas;

    // A sink that only remembers how much got through.
    struct CountingSink final : antwika::event::ITickEventSink
    {
        int seen = 0;

        void handle(const TickEvent &) override
        {
            ++seen;
        }
    };

    struct Harness
    {
        InputEventCodec codec;
        InputFold input{codec};
        ConsoleState console;
        CountingSink inner;
        ConsoleGatedSink gate{inner, console, input};

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

        // The fold first, as bootstrap() registers it.
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
} // namespace

TEST(ConsoleGatedSinkTest, ForwardsEverythingWhileTheConsoleIsAway)
{
    Harness harness;

    harness.feed(KeyPressed{.key = Key::Space});
    harness.feed(PointerButtonPressed{
        .button = MouseButton::Left, .position = {.x = 10, .y = 10}});
    harness.feedTick();

    EXPECT_EQ(harness.inner.seen, 3);
}

TEST(ConsoleGatedSinkTest, EngineTickAlwaysReachesTheInnerSink)
{
    Harness harness;
    harness.openFully();

    harness.feedTick();

    EXPECT_EQ(harness.inner.seen, 1);
}

TEST(ConsoleGatedSinkTest, EveryKeyEdgeIsTheConsolesWhileItIsOut)
{
    Harness harness;
    harness.openFully();

    harness.feed(KeyPressed{.key = Key::Space});
    harness.feed(KeyReleased{.key = Key::Space});

    EXPECT_EQ(harness.inner.seen, 0);
}

TEST(ConsoleGatedSinkTest, APressIsClaimedOnlyOverTheSheet)
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

TEST(ConsoleGatedSinkTest, AScrollFollowsTheFoldedPointer)
{
    Harness harness;
    harness.openFully();

    harness.feed(PointerMoved{.position = {.x = 10, .y = 10}});
    harness.feed(PointerScrolled{.vertical = 1});

    // The movement passed; the scroll under the sheet did not.
    EXPECT_EQ(harness.inner.seen, 1);

    harness.feed(PointerMoved{.position = {.x = 10, .y = 400}});
    harness.feed(PointerScrolled{.vertical = 1});

    EXPECT_EQ(harness.inner.seen, 3);
}

TEST(ConsoleGatedSinkTest, AReleaseOverTheSheetStillPasses)
{
    Harness harness;
    harness.openFully();

    // A drag let go over the console is still let go.
    harness.feed(PointerButtonReleased{
        .button = MouseButton::Left, .position = {.x = 10, .y = 10}});

    EXPECT_EQ(harness.inner.seen, 1);
}
