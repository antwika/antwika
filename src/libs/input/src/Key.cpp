#include "antwika/input/Key.hpp"

#include <array>
#include <string>
#include <string_view>

#include "antwika/input/InputError.hpp"

namespace antwika::input
{

    namespace
    {
        struct KeyName
        {
            Key key;
            std::string_view name;
        };

        // One table drives both directions.
        // So a name can never map to a key other than the one that made it.
        // Both directions search it, so its order carries no meaning.
        constexpr std::array<KeyName, kKeyCount> kKeyNames{{
            {Key::A, "A"},
            {Key::B, "B"},
            {Key::C, "C"},
            {Key::D, "D"},
            {Key::E, "E"},
            {Key::F, "F"},
            {Key::G, "G"},
            {Key::H, "H"},
            {Key::I, "I"},
            {Key::J, "J"},
            {Key::K, "K"},
            {Key::L, "L"},
            {Key::M, "M"},
            {Key::N, "N"},
            {Key::O, "O"},
            {Key::P, "P"},
            {Key::Q, "Q"},
            {Key::R, "R"},
            {Key::S, "S"},
            {Key::T, "T"},
            {Key::U, "U"},
            {Key::V, "V"},
            {Key::W, "W"},
            {Key::X, "X"},
            {Key::Y, "Y"},
            {Key::Z, "Z"},
            {Key::Digit0, "Digit0"},
            {Key::Digit1, "Digit1"},
            {Key::Digit2, "Digit2"},
            {Key::Digit3, "Digit3"},
            {Key::Digit4, "Digit4"},
            {Key::Digit5, "Digit5"},
            {Key::Digit6, "Digit6"},
            {Key::Digit7, "Digit7"},
            {Key::Digit8, "Digit8"},
            {Key::Digit9, "Digit9"},
            {Key::F1, "F1"},
            {Key::F2, "F2"},
            {Key::F3, "F3"},
            {Key::F4, "F4"},
            {Key::F5, "F5"},
            {Key::F6, "F6"},
            {Key::F7, "F7"},
            {Key::F8, "F8"},
            {Key::F9, "F9"},
            {Key::F10, "F10"},
            {Key::F11, "F11"},
            {Key::F12, "F12"},
            {Key::ArrowLeft, "ArrowLeft"},
            {Key::ArrowRight, "ArrowRight"},
            {Key::ArrowUp, "ArrowUp"},
            {Key::ArrowDown, "ArrowDown"},
            {Key::Escape, "Escape"},
            {Key::Enter, "Enter"},
            {Key::Space, "Space"},
            {Key::Tab, "Tab"},
            {Key::Backspace, "Backspace"},
            {Key::Delete, "Delete"},
            {Key::Insert, "Insert"},
            {Key::Home, "Home"},
            {Key::End, "End"},
            {Key::PageUp, "PageUp"},
            {Key::PageDown, "PageDown"},
            {Key::Minus, "Minus"},
            {Key::Equal, "Equal"},
            {Key::LeftBracket, "LeftBracket"},
            {Key::RightBracket, "RightBracket"},
            {Key::Backslash, "Backslash"},
            {Key::Semicolon, "Semicolon"},
            {Key::Apostrophe, "Apostrophe"},
            {Key::Grave, "Grave"},
            {Key::Comma, "Comma"},
            {Key::Period, "Period"},
            {Key::Slash, "Slash"},
            {Key::CapsLock, "CapsLock"},
            {Key::LeftShift, "LeftShift"},
            {Key::RightShift, "RightShift"},
            {Key::LeftControl, "LeftControl"},
            {Key::RightControl, "RightControl"},
            {Key::LeftAlt, "LeftAlt"},
            {Key::RightAlt, "RightAlt"},
            {Key::LeftSuper, "LeftSuper"},
            {Key::RightSuper, "RightSuper"},
        }};
    } // namespace

    std::string_view toString(Key key) noexcept
    {
        for (const auto &entry : kKeyNames)
        {
            if (entry.key == key)
            {
                return entry.name;
            }
        }

        return "Unknown";
    }

    Key keyFromString(std::string_view name)
    {
        for (const auto &entry : kKeyNames)
        {
            if (entry.name == name)
            {
                return entry.key;
            }
        }

        throw InputError("input: unknown key name '" + std::string(name)
                         + "'");
    }

} // namespace antwika::input
