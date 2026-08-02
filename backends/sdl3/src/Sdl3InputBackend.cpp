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
        // SDL's scancodes, not its virtual keycodes.
        // A Key names where a key is, and a scancode is that number.
        // A keycode is the layout's answer instead.
        // Reading one loses every key a non-US layout moves.
        // The key at [ on a Swedish board reports SDLK_ARING.
        // No row below names that, so it would arrive as nothing.
        // What a key types is the application's own table to keep.
        // apps/music_editor's EditorKeys is that table.
        // A recording holds the position rather than the character.
        // So a session replays the same whatever layout is set.
        constexpr auto kKeys = std::to_array<std::pair<SDL_Scancode, Key>>({
            {SDL_SCANCODE_A, Key::A},
            {SDL_SCANCODE_B, Key::B},
            {SDL_SCANCODE_C, Key::C},
            {SDL_SCANCODE_D, Key::D},
            {SDL_SCANCODE_E, Key::E},
            {SDL_SCANCODE_F, Key::F},
            {SDL_SCANCODE_G, Key::G},
            {SDL_SCANCODE_H, Key::H},
            {SDL_SCANCODE_I, Key::I},
            {SDL_SCANCODE_J, Key::J},
            {SDL_SCANCODE_K, Key::K},
            {SDL_SCANCODE_L, Key::L},
            {SDL_SCANCODE_M, Key::M},
            {SDL_SCANCODE_N, Key::N},
            {SDL_SCANCODE_O, Key::O},
            {SDL_SCANCODE_P, Key::P},
            {SDL_SCANCODE_Q, Key::Q},
            {SDL_SCANCODE_R, Key::R},
            {SDL_SCANCODE_S, Key::S},
            {SDL_SCANCODE_T, Key::T},
            {SDL_SCANCODE_U, Key::U},
            {SDL_SCANCODE_V, Key::V},
            {SDL_SCANCODE_W, Key::W},
            {SDL_SCANCODE_X, Key::X},
            {SDL_SCANCODE_Y, Key::Y},
            {SDL_SCANCODE_Z, Key::Z},
            {SDL_SCANCODE_0, Key::Digit0},
            {SDL_SCANCODE_1, Key::Digit1},
            {SDL_SCANCODE_2, Key::Digit2},
            {SDL_SCANCODE_3, Key::Digit3},
            {SDL_SCANCODE_4, Key::Digit4},
            {SDL_SCANCODE_5, Key::Digit5},
            {SDL_SCANCODE_6, Key::Digit6},
            {SDL_SCANCODE_7, Key::Digit7},
            {SDL_SCANCODE_8, Key::Digit8},
            {SDL_SCANCODE_9, Key::Digit9},
            {SDL_SCANCODE_F1, Key::F1},
            {SDL_SCANCODE_F2, Key::F2},
            {SDL_SCANCODE_F3, Key::F3},
            {SDL_SCANCODE_F4, Key::F4},
            {SDL_SCANCODE_F5, Key::F5},
            {SDL_SCANCODE_F6, Key::F6},
            {SDL_SCANCODE_F7, Key::F7},
            {SDL_SCANCODE_F8, Key::F8},
            {SDL_SCANCODE_F9, Key::F9},
            {SDL_SCANCODE_F10, Key::F10},
            {SDL_SCANCODE_F11, Key::F11},
            {SDL_SCANCODE_F12, Key::F12},
            {SDL_SCANCODE_LEFT, Key::ArrowLeft},
            {SDL_SCANCODE_RIGHT, Key::ArrowRight},
            {SDL_SCANCODE_UP, Key::ArrowUp},
            {SDL_SCANCODE_DOWN, Key::ArrowDown},
            {SDL_SCANCODE_ESCAPE, Key::Escape},
            {SDL_SCANCODE_RETURN, Key::Enter},
            {SDL_SCANCODE_SPACE, Key::Space},
            {SDL_SCANCODE_TAB, Key::Tab},
            {SDL_SCANCODE_BACKSPACE, Key::Backspace},
            {SDL_SCANCODE_DELETE, Key::Delete},
            {SDL_SCANCODE_INSERT, Key::Insert},
            {SDL_SCANCODE_HOME, Key::Home},
            {SDL_SCANCODE_END, Key::End},
            {SDL_SCANCODE_PAGEUP, Key::PageUp},
            {SDL_SCANCODE_PAGEDOWN, Key::PageDown},
            {SDL_SCANCODE_MINUS, Key::Minus},
            {SDL_SCANCODE_EQUALS, Key::Equal},
            {SDL_SCANCODE_LEFTBRACKET, Key::LeftBracket},
            {SDL_SCANCODE_RIGHTBRACKET, Key::RightBracket},
            {SDL_SCANCODE_BACKSLASH, Key::Backslash},
            {SDL_SCANCODE_SEMICOLON, Key::Semicolon},
            {SDL_SCANCODE_APOSTROPHE, Key::Apostrophe},
            {SDL_SCANCODE_GRAVE, Key::Grave},
            {SDL_SCANCODE_COMMA, Key::Comma},
            {SDL_SCANCODE_PERIOD, Key::Period},
            {SDL_SCANCODE_SLASH, Key::Slash},
            {SDL_SCANCODE_NONUSBACKSLASH, Key::IntlBackslash},
            {SDL_SCANCODE_CAPSLOCK, Key::CapsLock},
            {SDL_SCANCODE_LSHIFT, Key::LeftShift},
            {SDL_SCANCODE_RSHIFT, Key::RightShift},
            {SDL_SCANCODE_LCTRL, Key::LeftControl},
            {SDL_SCANCODE_RCTRL, Key::RightControl},
            {SDL_SCANCODE_LALT, Key::LeftAlt},
            {SDL_SCANCODE_RALT, Key::RightAlt},
            {SDL_SCANCODE_LGUI, Key::LeftSuper},
            {SDL_SCANCODE_RGUI, Key::RightSuper},
        });

        // Every Key has an SDL scancode, or some key could never arrive.
        static_assert(kKeys.size() == kKeyCount);

        [[nodiscard]] std::optional<Key> keyOf(SDL_Scancode scancode)
        {
            for (const auto &[sdlKey, key] : kKeys)
            {
                if (sdlKey == scancode)
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
                const auto key = keyOf(event.key.scancode);
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
                const auto key = keyOf(event.key.scancode);
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
