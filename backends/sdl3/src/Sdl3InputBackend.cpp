#include "Sdl3InputBackend.hpp"

#include <SDL3/SDL.h>

#include <array>
#include <cstdint>
#include <string>
#include <utility>

#include <antwika/input/InputError.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/KeyModifiers.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/input/Position.hpp>
#include <antwika/log/Level.hpp>

namespace antwika::input::sdl3
{

    using antwika::log::Level;

    namespace
    {
        // SDL's virtual keycodes, not its scancodes.
        // A replay stores the name of the key the user pressed.
        // That is the layout-aware one.
        constexpr auto kKeys = std::to_array<std::pair<SDL_Keycode, Key>>({
            {SDLK_A, Key::A},
            {SDLK_B, Key::B},
            {SDLK_C, Key::C},
            {SDLK_D, Key::D},
            {SDLK_E, Key::E},
            {SDLK_F, Key::F},
            {SDLK_G, Key::G},
            {SDLK_H, Key::H},
            {SDLK_I, Key::I},
            {SDLK_J, Key::J},
            {SDLK_K, Key::K},
            {SDLK_L, Key::L},
            {SDLK_M, Key::M},
            {SDLK_N, Key::N},
            {SDLK_O, Key::O},
            {SDLK_P, Key::P},
            {SDLK_Q, Key::Q},
            {SDLK_R, Key::R},
            {SDLK_S, Key::S},
            {SDLK_T, Key::T},
            {SDLK_U, Key::U},
            {SDLK_V, Key::V},
            {SDLK_W, Key::W},
            {SDLK_X, Key::X},
            {SDLK_Y, Key::Y},
            {SDLK_Z, Key::Z},
            {SDLK_0, Key::Digit0},
            {SDLK_1, Key::Digit1},
            {SDLK_2, Key::Digit2},
            {SDLK_3, Key::Digit3},
            {SDLK_4, Key::Digit4},
            {SDLK_5, Key::Digit5},
            {SDLK_6, Key::Digit6},
            {SDLK_7, Key::Digit7},
            {SDLK_8, Key::Digit8},
            {SDLK_9, Key::Digit9},
            {SDLK_F1, Key::F1},
            {SDLK_F2, Key::F2},
            {SDLK_F3, Key::F3},
            {SDLK_F4, Key::F4},
            {SDLK_F5, Key::F5},
            {SDLK_F6, Key::F6},
            {SDLK_F7, Key::F7},
            {SDLK_F8, Key::F8},
            {SDLK_F9, Key::F9},
            {SDLK_F10, Key::F10},
            {SDLK_F11, Key::F11},
            {SDLK_F12, Key::F12},
            {SDLK_LEFT, Key::ArrowLeft},
            {SDLK_RIGHT, Key::ArrowRight},
            {SDLK_UP, Key::ArrowUp},
            {SDLK_DOWN, Key::ArrowDown},
            {SDLK_ESCAPE, Key::Escape},
            {SDLK_RETURN, Key::Enter},
            {SDLK_SPACE, Key::Space},
            {SDLK_TAB, Key::Tab},
            {SDLK_BACKSPACE, Key::Backspace},
            {SDLK_DELETE, Key::Delete},
            {SDLK_INSERT, Key::Insert},
            {SDLK_HOME, Key::Home},
            {SDLK_END, Key::End},
            {SDLK_PAGEUP, Key::PageUp},
            {SDLK_PAGEDOWN, Key::PageDown},
            {SDLK_MINUS, Key::Minus},
            {SDLK_EQUALS, Key::Equal},
            {SDLK_LEFTBRACKET, Key::LeftBracket},
            {SDLK_RIGHTBRACKET, Key::RightBracket},
            {SDLK_BACKSLASH, Key::Backslash},
            {SDLK_SEMICOLON, Key::Semicolon},
            {SDLK_APOSTROPHE, Key::Apostrophe},
            {SDLK_GRAVE, Key::Grave},
            {SDLK_COMMA, Key::Comma},
            {SDLK_PERIOD, Key::Period},
            {SDLK_SLASH, Key::Slash},
            {SDLK_CAPSLOCK, Key::CapsLock},
            {SDLK_LSHIFT, Key::LeftShift},
            {SDLK_RSHIFT, Key::RightShift},
            {SDLK_LCTRL, Key::LeftControl},
            {SDLK_RCTRL, Key::RightControl},
            {SDLK_LALT, Key::LeftAlt},
            {SDLK_RALT, Key::RightAlt},
            {SDLK_LGUI, Key::LeftSuper},
            {SDLK_RGUI, Key::RightSuper},
        });

        // Every Key has an SDL keycode, or some key could never arrive.
        static_assert(kKeys.size() == kKeyCount);

        [[nodiscard]] std::optional<Key> keyOf(SDL_Keycode keycode)
        {
            for (const auto &[sdlKey, key] : kKeys)
            {
                if (sdlKey == keycode)
                {
                    return key;
                }
            }

            return std::nullopt;
        }

        [[nodiscard]] std::optional<MouseButton> buttonOf(std::uint8_t button)
        {
            switch (button)
            {
            case SDL_BUTTON_LEFT:
                return MouseButton::Left;
            case SDL_BUTTON_MIDDLE:
                return MouseButton::Middle;
            case SDL_BUTTON_RIGHT:
                return MouseButton::Right;
            case SDL_BUTTON_X1:
                return MouseButton::X1;
            case SDL_BUTTON_X2:
                return MouseButton::X2;
            default:
                return std::nullopt;
            }
        }

        [[nodiscard]] KeyModifiers modifiersOf(SDL_Keymod mod)
        {
            return KeyModifiers{
                .shift = (mod & SDL_KMOD_SHIFT) != 0,
                .control = (mod & SDL_KMOD_CTRL) != 0,
                .alt = (mod & SDL_KMOD_ALT) != 0,
                .super = (mod & SDL_KMOD_GUI) != 0};
        }

        // SDL reports a pointer in floats, for sub-pixel precision.
        // Truncating is what makes a recorded position an integer.
        // A replay then cannot depend on how a float rounded.
        [[nodiscard]] Position positionOf(float x, float y)
        {
            return Position{
                .x = static_cast<std::int32_t>(x),
                .y = static_cast<std::int32_t>(y)};
        }

        // A press and a release differ only in the type they report.
        template <typename Edge>
        [[nodiscard]] std::optional<InputEvent> buttonEdge(
            const SDL_MouseButtonEvent &event)
        {
            const auto button = buttonOf(event.button);

            if (!button)
            {
                return std::nullopt;
            }

            return Edge{
                .button = *button,
                .position = positionOf(event.x, event.y),
                .modifiers = modifiersOf(SDL_GetModState())};
        }

        [[nodiscard]] std::optional<InputEvent> translate(
            const SDL_Event &event)
        {
            switch (event.type)
            {
            case SDL_EVENT_KEY_DOWN:
            {
                const auto key = keyOf(event.key.key);
                if (!key)
                {
                    return std::nullopt;
                }
                return KeyPressed{
                    .key = *key,
                    .modifiers = modifiersOf(event.key.mod),
                    .repeat = event.key.repeat};
            }
            case SDL_EVENT_KEY_UP:
            {
                const auto key = keyOf(event.key.key);
                if (!key)
                {
                    return std::nullopt;
                }
                return KeyReleased{
                    .key = *key, .modifiers = modifiersOf(event.key.mod)};
            }
            case SDL_EVENT_MOUSE_MOTION:
                return PointerMoved{
                    .position = positionOf(event.motion.x, event.motion.y)};
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                return buttonEdge<PointerButtonPressed>(event.button);
            case SDL_EVENT_MOUSE_BUTTON_UP:
                return buttonEdge<PointerButtonReleased>(event.button);
            default:
                // The pump routes nothing else here.
                // A wheel event is the one case left.
                return PointerScrolled{
                    .horizontal = static_cast<std::int32_t>(event.wheel.x),
                    .vertical = static_cast<std::int32_t>(event.wheel.y)};
            }
        }
    } // namespace

    // The pump raises its own error type, which stops at this seam.
    // Above here, an input failure is only ever an InputError.
    Sdl3InputBackend::Sdl3InputBackend(ILogger &logger)
    {
        try
        {
            pump = antwika::sdl3::Sdl3Pump::acquire(logger);
        }
        catch (const antwika::sdl3::Sdl3Error &error)
        {
            throw InputError(std::string("input.") + error.what());
        }

        logger.log(Level::Debug, "input.sdl3: reading keyboard and mouse");
    }

    std::string_view Sdl3InputBackend::name() const
    {
        return "sdl3";
    }

    InputCapabilities Sdl3InputBackend::capabilities() const
    {
        return InputCapabilities{.keyboard = true, .pointer = true};
    }

    std::optional<InputEvent> Sdl3InputBackend::pollEvent()
    {
        while (const auto pending = pump->nextInputEvent())
        {
            if (const auto edge = translate(*pending))
            {
                return edge;
            }
        }

        return std::nullopt;
    }

} // namespace antwika::input::sdl3
