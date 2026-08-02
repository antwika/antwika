#include "Sdl3InputBackend.hpp"

#include <SDL3/SDL.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <optional>
#include <variant>

#include <antwika/input/InputEvent.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/input/Position.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

namespace antwika::input::sdl3
{

    using antwika::log::mocks::MockLogger;
    using ::testing::NiceMock;

    namespace
    {
        /**
         * @brief A backend over SDL's real queue, fed by hand.
         *
         * The conformance suite cannot assert that anything ever arrives,
         * because no framework offers a portable way to press a key. SDL
         * does offer a way to put an event on its own queue, though, which
         * is enough to check the one thing left: that an SDL event becomes
         * the edge it should.
         *
         * This is not a substitute for the conformance suite. It says
         * nothing about whether real hardware produces these events, only
         * about what this backend does with them once they arrive.
         */
        class Sdl3InputBackendTest : public ::testing::Test
        {
        protected:
            void SetUp() override
            {
                // What the window system posted on the way up is not ours.
                drain();
            }

            void push(SDL_Event event)
            {
                ASSERT_TRUE(SDL_PushEvent(&event)) << SDL_GetError();
            }

            [[nodiscard]] std::optional<InputEvent> nextEdge()
            {
                return backend.pollEvent();
            }

            void drain()
            {
                while (backend.pollEvent())
                {
                }
            }

            NiceMock<MockLogger> logger;
            Sdl3InputBackend backend{logger};
        };
    } // namespace

    TEST_F(Sdl3InputBackendTest, Name_IsSdl3)
    {
        EXPECT_EQ(backend.name(), "sdl3");
    }

    TEST_F(Sdl3InputBackendTest, Capabilities_ClaimBothDevices)
    {
        EXPECT_TRUE(backend.capabilities().keyboard);
        EXPECT_TRUE(backend.capabilities().pointer);
    }

    TEST_F(Sdl3InputBackendTest, PollEvent_TranslatesAPointerPress)
    {
        SDL_Event event{};
        event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
        event.button.button = SDL_BUTTON_LEFT;
        event.button.x = 412.75F;
        event.button.y = 118.25F;

        push(event);

        EXPECT_EQ(
            nextEdge(),
            (InputEvent{PointerButtonPressed{
                .button = MouseButton::Left,
                .position = Position{.x = 412, .y = 118},
                .modifiers = {}}}));
    }

    TEST_F(Sdl3InputBackendTest, PollEvent_TranslatesAPointerRelease)
    {
        SDL_Event event{};
        event.type = SDL_EVENT_MOUSE_BUTTON_UP;
        event.button.button = SDL_BUTTON_RIGHT;
        event.button.x = 1.0F;
        event.button.y = 2.0F;

        push(event);

        EXPECT_EQ(
            nextEdge(),
            (InputEvent{PointerButtonReleased{
                .button = MouseButton::Right,
                .position = Position{.x = 1, .y = 2},
                .modifiers = {}}}));
    }

    TEST_F(Sdl3InputBackendTest, PollEvent_TranslatesPointerMotion)
    {
        SDL_Event event{};
        event.type = SDL_EVENT_MOUSE_MOTION;
        event.motion.x = 640.0F;
        event.motion.y = 480.0F;

        push(event);

        EXPECT_EQ(
            nextEdge(),
            (InputEvent{
                PointerMoved{.position = Position{.x = 640, .y = 480}}}));
    }

    // A drag is a press, a run of motion, then a release.
    // Order has to survive the queue, or a drag would draw backwards.
    TEST_F(Sdl3InputBackendTest, PollEvent_KeepsADragInTheOrderItHappened)
    {
        for (const auto &[type, x] :
             {std::pair<SDL_EventType, float>{
                  SDL_EVENT_MOUSE_BUTTON_DOWN, 10.0F},
              {SDL_EVENT_MOUSE_MOTION, 20.0F},
              {SDL_EVENT_MOUSE_MOTION, 30.0F},
              {SDL_EVENT_MOUSE_BUTTON_UP, 30.0F}})
        {
            SDL_Event event{};
            event.type = type;
            event.button.button = SDL_BUTTON_LEFT;
            event.button.x = x;
            event.button.y = 5.0F;
            event.motion.x = x;
            event.motion.y = 5.0F;
            push(event);
        }

        const auto first = nextEdge();
        const auto second = nextEdge();
        const auto third = nextEdge();
        const auto fourth = nextEdge();

        ASSERT_TRUE(first && second && third && fourth);
        EXPECT_TRUE(std::holds_alternative<PointerButtonPressed>(*first));
        EXPECT_TRUE(std::holds_alternative<PointerMoved>(*second));
        EXPECT_EQ(
            std::get<PointerMoved>(*second).position,
            (Position{.x = 20, .y = 5}));
        EXPECT_TRUE(std::holds_alternative<PointerMoved>(*third));
        EXPECT_EQ(
            std::get<PointerMoved>(*third).position,
            (Position{.x = 30, .y = 5}));
        EXPECT_TRUE(std::holds_alternative<PointerButtonReleased>(*fourth));
    }

    TEST_F(Sdl3InputBackendTest, PollEvent_TranslatesAScrollInWholeNotches)
    {
        SDL_Event event{};
        event.type = SDL_EVENT_MOUSE_WHEEL;
        event.wheel.x = -1.0F;
        event.wheel.y = 3.0F;

        push(event);

        EXPECT_EQ(
            nextEdge(),
            (InputEvent{
                PointerScrolled{.horizontal = -1, .vertical = 3}}));
    }

    // A touchpad scrolls in fractions of a notch.
    // Two halves make a whole on the second event, not zero on each.
    TEST_F(Sdl3InputBackendTest, PollEvent_CarriesFractionalWheelNotches)
    {
        SDL_Event event{};
        event.type = SDL_EVENT_MOUSE_WHEEL;
        event.wheel.y = 0.5F;

        push(event);
        EXPECT_FALSE(nextEdge().has_value());

        push(event);
        EXPECT_EQ(
            nextEdge(),
            (InputEvent{
                PointerScrolled{.horizontal = 0, .vertical = 1}}));
    }

    // A flipped wheel arrives with its values negated.
    TEST_F(Sdl3InputBackendTest, PollEvent_HonoursAFlippedWheel)
    {
        SDL_Event event{};
        event.type = SDL_EVENT_MOUSE_WHEEL;
        event.wheel.y = 2.0F;
        event.wheel.direction = SDL_MOUSEWHEEL_FLIPPED;

        push(event);

        EXPECT_EQ(
            nextEdge(),
            (InputEvent{
                PointerScrolled{.horizontal = 0, .vertical = -2}}));
    }

    // AltGr arrives as SDL_KMOD_MODE, which is ISO_Level3_Shift.
    // The Swedish table's whole third column rides on it.
    TEST_F(Sdl3InputBackendTest, PollEvent_ReadsAltGrAsAlt)
    {
        SDL_Event event{};
        event.type = SDL_EVENT_KEY_DOWN;
        event.key.scancode = SDL_SCANCODE_4;
        event.key.mod = SDL_KMOD_MODE;

        push(event);

        EXPECT_EQ(
            nextEdge(),
            (InputEvent{KeyPressed{
                .key = Key::Digit4, .modifiers = {.alt = true}}}));
    }

    TEST_F(Sdl3InputBackendTest, PollEvent_TranslatesAKeyPress)
    {
        // The scancode, which is the field the backend translates.
        // The keycode is the layout's answer and is deliberately unread.
        SDL_Event event{};
        event.type = SDL_EVENT_KEY_DOWN;
        event.key.scancode = SDL_SCANCODE_ESCAPE;
        event.key.mod = SDL_KMOD_LSHIFT;
        event.key.repeat = true;

        push(event);

        EXPECT_EQ(
            nextEdge(),
            (InputEvent{KeyPressed{
                .key = Key::Escape,
                .modifiers = {.shift = true},
                .repeat = true}}));
    }

    TEST_F(Sdl3InputBackendTest, PollEvent_TranslatesAKeyRelease)
    {
        SDL_Event event{};
        event.type = SDL_EVENT_KEY_UP;
        event.key.scancode = SDL_SCANCODE_A;
        event.key.mod = SDL_KMOD_LCTRL | SDL_KMOD_RALT;

        push(event);

        EXPECT_EQ(
            nextEdge(),
            (InputEvent{KeyReleased{
                .key = Key::A,
                .modifiers = {.control = true, .alt = true}}}));
    }

    // A key this vocabulary has no word for is dropped.
    // Polling goes on, so the queue still drains.
    TEST_F(Sdl3InputBackendTest, PollEvent_SkipsAKeyNoKeyNames)
    {
        SDL_Event unnamed{};
        unnamed.type = SDL_EVENT_KEY_DOWN;
        unnamed.key.scancode = SDL_SCANCODE_F24;

        SDL_Event named{};
        named.type = SDL_EVENT_KEY_DOWN;
        named.key.scancode = SDL_SCANCODE_SPACE;

        push(unnamed);
        push(named);

        EXPECT_EQ(
            nextEdge(),
            (InputEvent{KeyPressed{.key = Key::Space}}));
        EXPECT_FALSE(nextEdge().has_value());
    }

    TEST_F(Sdl3InputBackendTest, PollEvent_SkipsAButtonNoButtonNames)
    {
        SDL_Event unnamed{};
        unnamed.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
        unnamed.button.button = 9;

        push(unnamed);

        EXPECT_FALSE(nextEdge().has_value());
    }

    // A window event belongs to the graphics seam.
    // It must not come back out of this one -- see Sdl3Pump.
    TEST_F(Sdl3InputBackendTest, PollEvent_LeavesWindowEventsAlone)
    {
        SDL_Event closing{};
        closing.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED;
        closing.window.windowID = 1;

        push(closing);

        EXPECT_FALSE(nextEdge().has_value());
    }

    TEST_F(Sdl3InputBackendTest, PollEvent_DrainsToAnEmptyQueue)
    {
        SDL_Event event{};
        event.type = SDL_EVENT_MOUSE_MOTION;
        event.motion.x = 1.0F;
        event.motion.y = 1.0F;

        push(event);

        EXPECT_TRUE(nextEdge().has_value());
        EXPECT_FALSE(nextEdge().has_value());
        EXPECT_FALSE(nextEdge().has_value());
    }

} // namespace antwika::input::sdl3
