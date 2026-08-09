#include <gtest/gtest.h>

#include <variant>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/input/InputError.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/input/Position.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/console/InputFold.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::console::InputFold;
using antwika::gfx::Point;
using antwika::input::InputError;
using antwika::input::InputEvent;
using antwika::input::InputEventCodec;
using antwika::input::KeyPressed;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::input::PointerMoved;
using antwika::input::Position;
using antwika::time::Tick;

namespace
{
    class InputFoldTest : public ::testing::Test
    {
    protected:
        void send(const InputEvent &event, Tick tick = 0)
        {
            fold.handle(
                TickEvent{.tick = tick, .event = codec.encode(event)});
        }

        void tick(Tick at = 0)
        {
            fold.handle(
                TickEvent{
                    .tick = at,
                    .event =
                        Event{.name = antwika::engine::events::kTick}});
        }

        InputEventCodec codec;

        InputFold fold{codec};
    };
}

TEST_F(InputFoldTest, Located_IsFalseUntilSomethingSaysWhereItIs)
{
    EXPECT_FALSE(fold.located());

    send(KeyPressed{});

    EXPECT_FALSE(fold.located());

    send(PointerMoved{.position = Position{.x = 4, .y = 5}});

    EXPECT_TRUE(fold.located());
}

TEST_F(InputFoldTest, Pointer_FollowsTheFoldedPosition)
{
    send(PointerMoved{.position = Position{.x = 4, .y = 5}});

    EXPECT_EQ(fold.pointer(), (Point{.x = 4, .y = 5}));
    EXPECT_EQ(fold.state().mouse().position(), (Position{.x = 4, .y = 5}));
}

TEST_F(InputFoldTest, PointerBefore_IsWhereItWasOneEventAgo)
{
    send(PointerMoved{.position = Position{.x = 4, .y = 5}});
    send(PointerMoved{.position = Position{.x = 9, .y = 11}});

    EXPECT_EQ(fold.pointerBefore(), (Point{.x = 4, .y = 5}));
    EXPECT_EQ(fold.pointer(), (Point{.x = 9, .y = 11}));
}

TEST_F(InputFoldTest, Current_HoldsTheEventJustFolded)
{
    send(PointerButtonPressed{.button = MouseButton::Right});

    ASSERT_TRUE(fold.current().has_value());
    EXPECT_TRUE(
        std::holds_alternative<PointerButtonPressed>(*fold.current()));
}

TEST_F(InputFoldTest, Current_IsEmptyForAnEventThatIsNotInput)
{
    send(PointerMoved{.position = Position{.x = 4, .y = 5}});
    tick();

    EXPECT_FALSE(fold.current().has_value());

    fold.handle(
        TickEvent{.tick = 0, .event = Event{.name = "game.started"}});

    EXPECT_FALSE(fold.current().has_value());
}

TEST_F(InputFoldTest, Handle_KeepsThisTicksEdgesUntilTheNextTickStarts)
{
    send(PointerButtonPressed{.button = MouseButton::Left});
    tick();

    EXPECT_TRUE(fold.state().mouse().wasPressed(MouseButton::Left));

    tick(1);

    EXPECT_FALSE(fold.state().mouse().wasPressed(MouseButton::Left));
    EXPECT_TRUE(fold.state().mouse().isDown(MouseButton::Left));
}

TEST_F(InputFoldTest, Handle_LetsAMalformedInputPayloadThrough)
{
    EXPECT_THROW(
        fold.handle(
            TickEvent{
                .tick = 0,
                .event = Event{
                    .name = "input.pointer_down",
                    .payload = "not json"}}),
        InputError);
}
