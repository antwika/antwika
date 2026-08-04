#include <gtest/gtest.h>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/input/Position.hpp>

#include "antwika/game/Action.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/HotkeySink.hpp"
#include "antwika/game/InputFold.hpp"
#include "antwika/game/KeyBindings.hpp"
#include "antwika/game/OptionsState.hpp"
#include "antwika/game/PauseState.hpp"
#include "antwika/game/ViewCommands.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::game::Action;
using antwika::game::Camera;
using antwika::game::HotkeySink;
using antwika::game::InputFold;
using antwika::game::ViewCommands;
using antwika::game::kDefaultBindings;
using antwika::game::OptionsState;
using antwika::game::PauseState;
using antwika::gfx::Point;
using antwika::input::InputEvent;
using antwika::input::InputEventCodec;
using antwika::input::Key;
using antwika::input::KeyPressed;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::input::Position;

namespace
{
    constexpr Point kHome{.x = 100, .y = 40};

    class HotkeySinkTest : public ::testing::Test
    {
    protected:
        // Through the fold first, as bootstrap() registers it.
        void send(const InputEvent &event)
        {
            const TickEvent timed{
                .tick = 0, .event = codec.encode(event)};

            input.handle(timed);
            sink.handle(timed);
        }

        void press(Key key, bool repeat = false)
        {
            send(KeyPressed{.key = key, .repeat = repeat});
        }

        void tick()
        {
            const TickEvent timed{
                .tick = 0,
                .event = Event{.name = antwika::engine::events::kTick}};

            input.handle(timed);
            sink.handle(timed);
        }

        InputEventCodec codec;
        OptionsState options;
        InputFold input{codec};
        Camera camera{kHome};
        PauseState pause;
        ViewCommands view{camera, pause, camera};
        HotkeySink sink{options, input, view};
    };
} // namespace

TEST_F(HotkeySinkTest, ThePauseKeyHoldsTheRunAndLetsItGo)
{
    press(kDefaultBindings.keyFor(Action::Pause));
    EXPECT_TRUE(pause.paused());

    press(kDefaultBindings.keyFor(Action::Pause));
    EXPECT_FALSE(pause.paused());
}

TEST_F(HotkeySinkTest, TheZoomKeysMoveTheCamera)
{
    const auto opened = camera.zoomLevel();

    press(kDefaultBindings.keyFor(Action::ZoomIn));
    EXPECT_GT(camera.zoomLevel(), opened);

    press(kDefaultBindings.keyFor(Action::ZoomOut));
    EXPECT_EQ(camera.zoomLevel(), opened);
}

TEST_F(HotkeySinkTest, TheResetKeyPutsTheCameraBackWhereTheRunOpenedIt)
{
    camera.panBy(64, 64);
    camera.zoomIn();

    press(kDefaultBindings.keyFor(Action::ResetView));

    EXPECT_EQ(camera, Camera(kHome));
}

// A binding is what a key means, so rebinding is what it means now.
TEST_F(HotkeySinkTest, ARebindingIsWhatTheKeyMeansFromThenOn)
{
    EXPECT_EQ(
        options.apply(Action::Pause, Key::J),
        antwika::game::BindOutcome::Bound);

    press(kDefaultBindings.keyFor(Action::Pause));
    EXPECT_FALSE(pause.paused());

    press(Key::J);
    EXPECT_TRUE(pause.paused());
}

TEST_F(HotkeySinkTest, AKeyNothingIsBoundToDoesNothing)
{
    press(Key::J);

    EXPECT_FALSE(pause.paused());
    EXPECT_EQ(camera, Camera(kHome));
}

// A repeat is not a fresh press, so holding pause holds the run once.
TEST_F(HotkeySinkTest, AHeldKeyIsOneAnswerRatherThanAFlicker)
{
    press(kDefaultBindings.keyFor(Action::Pause));
    press(kDefaultBindings.keyFor(Action::Pause), true);
    press(kDefaultBindings.keyFor(Action::Pause), true);

    EXPECT_TRUE(pause.paused());
}

TEST_F(HotkeySinkTest, InputThatIsNotAKeyPressIsIgnored)
{
    send(
        PointerButtonPressed{
            .button = MouseButton::Left,
            .position = Position{.x = 8, .y = 8}});

    EXPECT_FALSE(pause.paused());
    EXPECT_EQ(camera, Camera(kHome));
}

// A tick carries nothing the fold could hand over.
TEST_F(HotkeySinkTest, AnEventThatIsNotInputIsIgnored)
{
    tick();

    EXPECT_FALSE(pause.paused());
    EXPECT_EQ(camera, Camera(kHome));
}
