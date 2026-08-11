#include "RaylibInputBackend.hpp"

#include <raylib.h>

#include <utility>

#include <antwika/log/Level.hpp>

#include "RaylibFrame.hpp"

namespace antwika::input::raylib
{

    using antwika::log::Level;

    namespace
    {
        constexpr std::array<std::pair<MouseButton, int>, kMouseButtonCount>
            kButtons{{
                {MouseButton::Left, MOUSE_BUTTON_LEFT},
                {MouseButton::Middle, MOUSE_BUTTON_MIDDLE},
                {MouseButton::Right, MOUSE_BUTTON_RIGHT},
                {MouseButton::X1, MOUSE_BUTTON_SIDE},
                {MouseButton::X2, MOUSE_BUTTON_EXTRA},
            }};

        [[nodiscard]] Position positionNow()
        {
            const Vector2 position = GetMousePosition();

            return Position{
                .x = static_cast<std::int32_t>(position.x),
                .y = static_cast<std::int32_t>(position.y)};
        }

        constexpr std::array<std::pair<Key, int>, 93> kKeys{{
            {Key::A, KEY_A},
            {Key::B, KEY_B},
            {Key::C, KEY_C},
            {Key::D, KEY_D},
            {Key::E, KEY_E},
            {Key::F, KEY_F},
            {Key::G, KEY_G},
            {Key::H, KEY_H},
            {Key::I, KEY_I},
            {Key::J, KEY_J},
            {Key::K, KEY_K},
            {Key::L, KEY_L},
            {Key::M, KEY_M},
            {Key::N, KEY_N},
            {Key::O, KEY_O},
            {Key::P, KEY_P},
            {Key::Q, KEY_Q},
            {Key::R, KEY_R},
            {Key::S, KEY_S},
            {Key::T, KEY_T},
            {Key::U, KEY_U},
            {Key::V, KEY_V},
            {Key::W, KEY_W},
            {Key::X, KEY_X},
            {Key::Y, KEY_Y},
            {Key::Z, KEY_Z},
            {Key::Digit0, KEY_ZERO},
            {Key::Digit1, KEY_ONE},
            {Key::Digit2, KEY_TWO},
            {Key::Digit3, KEY_THREE},
            {Key::Digit4, KEY_FOUR},
            {Key::Digit5, KEY_FIVE},
            {Key::Digit6, KEY_SIX},
            {Key::Digit7, KEY_SEVEN},
            {Key::Digit8, KEY_EIGHT},
            {Key::Digit9, KEY_NINE},
            {Key::Keypad0, KEY_KP_0},
            {Key::Keypad1, KEY_KP_1},
            {Key::Keypad2, KEY_KP_2},
            {Key::Keypad3, KEY_KP_3},
            {Key::Keypad4, KEY_KP_4},
            {Key::Keypad5, KEY_KP_5},
            {Key::Keypad6, KEY_KP_6},
            {Key::Keypad7, KEY_KP_7},
            {Key::Keypad8, KEY_KP_8},
            {Key::Keypad9, KEY_KP_9},
            {Key::F1, KEY_F1},
            {Key::F2, KEY_F2},
            {Key::F3, KEY_F3},
            {Key::F4, KEY_F4},
            {Key::F5, KEY_F5},
            {Key::F6, KEY_F6},
            {Key::F7, KEY_F7},
            {Key::F8, KEY_F8},
            {Key::F9, KEY_F9},
            {Key::F10, KEY_F10},
            {Key::F11, KEY_F11},
            {Key::F12, KEY_F12},
            {Key::ArrowLeft, KEY_LEFT},
            {Key::ArrowRight, KEY_RIGHT},
            {Key::ArrowUp, KEY_UP},
            {Key::ArrowDown, KEY_DOWN},
            {Key::Escape, KEY_ESCAPE},
            {Key::Enter, KEY_ENTER},
            {Key::Space, KEY_SPACE},
            {Key::Tab, KEY_TAB},
            {Key::Backspace, KEY_BACKSPACE},
            {Key::Delete, KEY_DELETE},
            {Key::Insert, KEY_INSERT},
            {Key::Home, KEY_HOME},
            {Key::End, KEY_END},
            {Key::PageUp, KEY_PAGE_UP},
            {Key::PageDown, KEY_PAGE_DOWN},
            {Key::Minus, KEY_MINUS},
            {Key::Equal, KEY_EQUAL},
            {Key::LeftBracket, KEY_LEFT_BRACKET},
            {Key::RightBracket, KEY_RIGHT_BRACKET},
            {Key::Backslash, KEY_BACKSLASH},
            {Key::Semicolon, KEY_SEMICOLON},
            {Key::Apostrophe, KEY_APOSTROPHE},
            {Key::Grave, KEY_GRAVE},
            {Key::Comma, KEY_COMMA},
            {Key::Period, KEY_PERIOD},
            {Key::Slash, KEY_SLASH},
            {Key::CapsLock, KEY_CAPS_LOCK},
            {Key::LeftShift, KEY_LEFT_SHIFT},
            {Key::RightShift, KEY_RIGHT_SHIFT},
            {Key::LeftControl, KEY_LEFT_CONTROL},
            {Key::RightControl, KEY_RIGHT_CONTROL},
            {Key::LeftAlt, KEY_LEFT_ALT},
            {Key::RightAlt, KEY_RIGHT_ALT},
            {Key::LeftSuper, KEY_LEFT_SUPER},
            {Key::RightSuper, KEY_RIGHT_SUPER},
        }};

        [[nodiscard]] KeyModifiers modifiersNow()
        {
            return KeyModifiers{
                .shift = IsKeyDown(KEY_LEFT_SHIFT)
                         || IsKeyDown(KEY_RIGHT_SHIFT),
                .control = IsKeyDown(KEY_LEFT_CONTROL)
                           || IsKeyDown(KEY_RIGHT_CONTROL),
                .alt = IsKeyDown(KEY_LEFT_ALT)
                       || IsKeyDown(KEY_RIGHT_ALT),
                .super = IsKeyDown(KEY_LEFT_SUPER)
                         || IsKeyDown(KEY_RIGHT_SUPER)};
        }

        [[nodiscard]] std::optional<Key> keyFromCode(const int code)
        {
            for (const auto &[key, raylibKey] : kKeys)
            {
                if (raylibKey == code)
                {
                    return key;
                }
            }

            return std::nullopt;
        }
    }

    RaylibInputBackend::RaylibInputBackend(ILogger &logger)
    {
        logger.log(Level::Debug, "input.raylib: reading the mouse");
    }

    std::string_view RaylibInputBackend::name() const
    {
        return "raylib";
    }

    InputCapabilities RaylibInputBackend::capabilities() const
    {
        return InputCapabilities{.keyboard = true, .pointer = true};
    }

    std::optional<InputEvent> RaylibInputBackend::pollEvent()
    {
        if (pending.empty())
        {
            sample();
        }

        if (pending.empty())
        {
            return std::nullopt;
        }

        auto event = pending.front();
        pending.pop_front();

        return event;
    }

    void RaylibInputBackend::sample()
    {
        if (!IsWindowReady())
        {
            return;
        }

        const auto position = positionNow();

        if (!lastPosition)
        {
            lastPosition = position;
        }
        else if (*lastPosition != position)
        {
            lastPosition = position;
            pending.push_back(PointerMoved{.position = position});
        }

        for (const auto &[button, raylibButton] : kButtons)
        {
            const bool down = IsMouseButtonDown(raylibButton);
            auto &wasDown = held[mouseButtonIndex(button)];

            if (down == wasDown)
            {
                continue;
            }

            wasDown = down;

            if (down)
            {
                pending.push_back(PointerButtonPressed{
                    .button = button,
                    .position = position,
                    .modifiers = modifiersNow()});
            }
            else
            {
                pending.push_back(PointerButtonReleased{
                    .button = button,
                    .position = position,
                    .modifiers = modifiersNow()});
            }
        }

        for (int code = GetKeyPressed(); code != 0;
             code = GetKeyPressed())
        {
            const auto key = keyFromCode(code);

            if (!key.has_value())
            {
                continue;
            }

            auto &wasDown = heldKeys[enums::index(*key)];

            pending.push_back(KeyPressed{
                .key = *key,
                .modifiers = modifiersNow(),
                .repeat = wasDown});
            wasDown = true;
        }

        for (const auto &[key, raylibKey] : kKeys)
        {
            auto &wasDown = heldKeys[enums::index(key)];

            if (!wasDown || IsKeyDown(raylibKey))
            {
                continue;
            }

            wasDown = false;
            pending.push_back(KeyReleased{
                .key = key, .modifiers = modifiersNow()});
        }

        const auto frame = antwika::raylib::frameCount();

        if (frame == wheelFrame)
        {
            return;
        }

        wheelFrame = frame;

        const Vector2 wheel = GetMouseWheelMoveV();

        remainderX += wheel.x;
        remainderY += wheel.y;

        const auto horizontal = static_cast<std::int32_t>(remainderX);
        const auto vertical = static_cast<std::int32_t>(remainderY);

        if (horizontal == 0 && vertical == 0)
        {
            return;
        }

        remainderX -= static_cast<float>(horizontal);
        remainderY -= static_cast<float>(vertical);

        pending.push_back(PointerScrolled{
            .horizontal = horizontal, .vertical = vertical});
    }

}
