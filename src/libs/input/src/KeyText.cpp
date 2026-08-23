#include "antwika/input/KeyText.hpp"

#include <array>
#include <cstdint>

namespace antwika::input
{

    namespace
    {
        [[nodiscard]] std::uint8_t rankOf(const Key key)
        {
            return static_cast<std::uint8_t>(key);
        }

        struct KeyChars final
        {
            Key key;

            char plain;

            char shiftedChar;
        };

        constexpr std::array<KeyChars, 11> kMarkedKeys{
            KeyChars{Key::Minus, '-', '_'},
            KeyChars{Key::Equal, '=', '+'},
            KeyChars{Key::LeftBracket, '[', '{'},
            KeyChars{Key::RightBracket, ']', '}'},
            KeyChars{Key::Backslash, '\\', '|'},
            KeyChars{Key::Semicolon, ';', ':'},
            KeyChars{Key::Apostrophe, '\'', '"'},
            KeyChars{Key::Grave, '`', '~'},
            KeyChars{Key::Comma, ',', '<'},
            KeyChars{Key::Period, '.', '>'},
            KeyChars{Key::Slash, '/', '?'}};

        constexpr std::array<char, 10> kShiftedDigits{
            ')', '!', '@', '#', '$', '%', '^', '&', '*', '('};
    }

    std::string getCharTypedBy(const Key key, const bool shiftHeld)
    {
        const auto rank = rankOf(key);

        if (rank >= rankOf(Key::A) && rank <= rankOf(Key::Z))
        {
            const auto letterIndex = rank - rankOf(Key::A);

            return std::string(
                1,
                static_cast<char>((shiftHeld ? 'A' : 'a') + letterIndex));
        }

        if (rank >= rankOf(Key::Digit0) && rank <= rankOf(Key::Digit9))
        {
            const auto digitIndex = rank - rankOf(Key::Digit0);

            return std::string(
                1,
                shiftHeld ? kShiftedDigits.at(digitIndex)
                          : static_cast<char>('0' + digitIndex));
        }

        for (const auto &mark : kMarkedKeys)
        {
            if (mark.key == key)
            {
                return std::string(
                    1, shiftHeld ? mark.shiftedChar : mark.plain);
            }
        }

        if (key == Key::Space)
        {
            return " ";
        }

        return {};
    } // GCOVR_EXCL_LINE

}
