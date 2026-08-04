#include "antwika/console/Typing.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace antwika::console
{

    using antwika::input::Key;

    namespace
    {
        // What shift over each digit types on a Swedish board.
        // The currency sign over 4 is not ASCII, so it types nothing.
        constexpr std::array<char, 10> kSwedishShiftedDigits{
            '=', '!', '"', '#', '\0', '%', '&', '/', '(', ')'};

        [[nodiscard]] char englishPunctuation(
            Key key, bool shift) noexcept
        {
            switch (key)
            {
            case Key::Minus:
                // The shifted hyphen, since a file name often wants one.
                return shift ? '_' : '-';
            case Key::Period:
                return shift ? '\0' : '.';
            default:
                break;
            }

            return '\0';
        }

        [[nodiscard]] char swedishPunctuation(
            Key key, bool shift) noexcept
        {
            // Named by where the key *is* -- the American position.
            // What it says is what a Swedish board prints there.
            switch (key)
            {
            case Key::Minus:
                return shift ? '?' : '+';
            case Key::Slash:
                return shift ? '_' : '-';
            case Key::Period:
                return shift ? ':' : '.';
            case Key::Comma:
                return shift ? ';' : ',';
            default:
                break;
            }

            return '\0';
        }
    } // namespace

    char typedCharacterFor(
        Key key, bool shift, KeyboardLayout layout) noexcept
    {
        const auto index = static_cast<std::uint8_t>(key);

        // The digits are asked about first, and that is not arbitrary.
        // Key::A is the enumeration's zero, so "at least A" is folded.
        // Letters first would leave "at least Digit0" always true here.
        // That is a branch no test could take.
        // And Key.hpp says the order may be changed anyway.
        if (key >= Key::Digit0 && key <= Key::Digit9)
        {
            const auto digit =
                index - static_cast<std::uint8_t>(Key::Digit0);

            if (!shift)
            {
                return static_cast<char>('0' + digit);
            }

            // Only the Swedish board says what shift over one types.
            // The American row's symbols are not what save names need.
            return layout == KeyboardLayout::Swedish
                       ? kSwedishShiftedDigits[static_cast<std::size_t>(
                             digit)]
                       : '\0';
        }

        if (key >= Key::A && key <= Key::Z)
        {
            const auto offset =
                index - static_cast<std::uint8_t>(Key::A);
            return static_cast<char>((shift ? 'A' : 'a') + offset);
        }

        if (key == Key::Space)
        {
            return ' ';
        }

        return layout == KeyboardLayout::Swedish
                   ? swedishPunctuation(key, shift)
                   : englishPunctuation(key, shift);
    }

    std::optional<antwika::ui::Key> consoleKeyFor(Key key) noexcept
    {
        switch (key)
        {
        case Key::Backspace:
            return antwika::ui::Key::Backspace;
        case Key::ArrowLeft:
            return antwika::ui::Key::MoveLeft;
        case Key::ArrowRight:
            return antwika::ui::Key::MoveRight;
        default:
            break;
        }

        return std::nullopt;
    }

} // namespace antwika::console
