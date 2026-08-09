#include "antwika/input/Key.hpp"

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

#include "antwika/input/InputError.hpp"

namespace antwika::input
{

    namespace
    {
        struct KeyName final
        {
            Key key;
            std::string_view name;
        };

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
            {Key::Keypad0, "Keypad0"},
            {Key::Keypad1, "Keypad1"},
            {Key::Keypad2, "Keypad2"},
            {Key::Keypad3, "Keypad3"},
            {Key::Keypad4, "Keypad4"},
            {Key::Keypad5, "Keypad5"},
            {Key::Keypad6, "Keypad6"},
            {Key::Keypad7, "Keypad7"},
            {Key::Keypad8, "Keypad8"},
            {Key::Keypad9, "Keypad9"},
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
            {Key::IntlBackslash, "IntlBackslash"},
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

        [[nodiscard]] consteval bool namesEveryKeyExactlyOnce()
        {
            for (std::size_t index = 0; index < kKeyCount; ++index)
            {
                std::size_t rows = 0;

                for (const auto &entry : kKeyNames)
                {
                    if (keyIndex(entry.key) == index)
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

        [[nodiscard]] consteval bool namesEveryKeyDistinctly()
        {
            const auto count = kKeyNames.size();

            for (std::size_t left = 0; left < count; ++left)
            {
                for (std::size_t right = left + 1; right < count; ++right)
                {
                    if (kKeyNames[left].name == kKeyNames[right].name)
                    {
                        return false;
                    }
                }
            }

            return true;
        }

        static_assert(
            namesEveryKeyExactlyOnce(),
            "kKeyNames must name every Key exactly once");
        static_assert(
            namesEveryKeyDistinctly(),
            "kKeyNames must not give two keys the same name");
    }

    std::string_view toString(Key key)
    {
        for (const auto &entry : kKeyNames)
        {
            if (entry.key == key)
            {
                return entry.name;
            }
        }

        throw InputError(
            "input: no name for key " + std::to_string(keyIndex(key)));
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

}
