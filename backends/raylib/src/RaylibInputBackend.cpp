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

        [[nodiscard]] Position getPositionNow()
        {
            const Vector2 position = GetMousePosition();

            return Position{
                .x = static_cast<std::int32_t>(position.x),
                .y = static_cast<std::int32_t>(position.y)};
        }

        constexpr int kUnmappedKey = KEY_NULL;

        constexpr std::array<std::pair<Key, int>, kKeyCount> kKeys{{
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
            {Key::KeypadAdd, KEY_KP_ADD},
            {Key::KeypadSubtract, KEY_KP_SUBTRACT},
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
            {Key::IntlBackslash, kUnmappedKey},
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

        [[nodiscard]] consteval bool mapsEveryKeyExactlyOnce()
        {
            for (std::size_t index = 0; index < kKeyCount; ++index)
            {
                std::size_t rows = 0;

                for (const auto &entry : kKeys)
                {
                    if (getKeyIndex(entry.first) == index)
                    {
                        ++rows;
                    }
                }

                if (rows != 1)
                {
                    return false;
                }
            }

            return true;
        }

        static_assert(
            mapsEveryKeyExactlyOnce(),
            "kKeys must give every Key a raylib code exactly once, "
            "kUnmappedKey where raylib has none");

        [[nodiscard]] KeyModifiers getModifiersNow()
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

        [[nodiscard]] std::optional<Key> getKeyFromCode(const int code)
        {
            for (const auto &[key, raylibKey] : kKeys)
            {
                if (raylibKey != kUnmappedKey && raylibKey == code)
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

    std::string_view RaylibInputBackend::getName() const
    {
        return "raylib";
    }

    InputCapabilities RaylibInputBackend::getCapabilities() const
    {
        return InputCapabilities{.keyboard = true, .pointer = true};
    }

    std::optional<InputEvent> RaylibInputBackend::pollEvent()
    {
        if (pendingEvents.empty())
        {
            pollDevices();
        }

        if (pendingEvents.empty())
        {
            return std::nullopt;
        }

        auto event = pendingEvents.front();
        pendingEvents.pop_front();

        return event;
    }

    void RaylibInputBackend::pollDevices()
    {
        if (!IsWindowReady())
        {
            return;
        }

        const auto position = getPositionNow();

        if (!lastPosition)
        {
            lastPosition = position;
        }
        else if (*lastPosition != position)
        {
            lastPosition = position;
            pendingEvents.push_back(PointerMoved{.position = position});
        }

        for (const auto &[button, raylibButton] : kButtons)
        {
            const bool down = IsMouseButtonDown(raylibButton);
            auto &wasDown = heldButtons[getMouseButtonIndex(button)];

            if (down == wasDown)
            {
                continue;
            }

            wasDown = down;

            if (down)
            {
                pendingEvents.push_back(PointerButtonPressed{
                    .button = button,
                    .position = position,
                    .modifiers = getModifiersNow()});
            }
            else
            {
                pendingEvents.push_back(PointerButtonReleased{
                    .button = button,
                    .position = position,
                    .modifiers = getModifiersNow()});
            }
        }

        for (int code = GetKeyPressed(); code != 0;
             code = GetKeyPressed())
        {
            const auto key = getKeyFromCode(code);

            if (!key.has_value())
            {
                continue;
            }

            auto &wasDown = heldKeys[getKeyIndex(*key)];

            pendingEvents.push_back(KeyPressed{
                .key = *key,
                .modifiers = getModifiersNow(),
                .repeat = wasDown});
            wasDown = true;
        }

        for (const auto &[key, raylibKey] : kKeys)
        {
            auto &wasDown = heldKeys[getKeyIndex(key)];

            if (raylibKey == kUnmappedKey || !wasDown
                || IsKeyDown(raylibKey))
            {
                continue;
            }

            wasDown = false;
            pendingEvents.push_back(KeyReleased{
                .key = key, .modifiers = getModifiersNow()});
        }

        const auto frame = antwika::raylib::getFrameCount();

        if (frame == lastWheelFrame)
        {
            return;
        }

        lastWheelFrame = frame;

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

        pendingEvents.push_back(PointerScrolled{
            .horizontal = horizontal, .vertical = vertical});
    }

}
