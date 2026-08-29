#include "antwika/ui/TextWrap.hpp"

#include <antwika/gfx/Glyphs.hpp>

namespace antwika::ui
{

    namespace
    {
        [[nodiscard]] std::size_t getPastSpaces(
            const std::string_view run, std::size_t charIndex)
        {
            while (charIndex < run.size() && run.at(charIndex) == ' ')
            {
                ++charIndex;
            }

            return charIndex;
        }

        [[nodiscard]] std::string_view getTrimmedText(
            const std::string_view piece)
        {
            return piece.substr(
                0, piece.find_last_not_of(' ') + 1);
        }

        void wrapRun(
            const std::string_view run,
            const std::size_t columns,
            std::vector<std::string_view> &lines)
        {
            auto charIndex = getPastSpaces(run, 0);

            while (run.size() - charIndex > columns)
            {
                const auto window = run.substr(charIndex, columns + 1);
                const auto space = window.find_last_of(' ');

                if (space == std::string_view::npos)
                {
                    lines.push_back(run.substr(charIndex, columns));
                    charIndex += columns;
                }
                else
                {
                    lines.push_back(getTrimmedText(run.substr(charIndex, space)));
                    charIndex += space;
                }

                charIndex = getPastSpaces(run, charIndex);
            }

            lines.push_back(getTrimmedText(run.substr(charIndex)));
        }
    }

    std::size_t getWrapColumns(
        const Theme &theme, const std::uint32_t width) noexcept
    {
        const auto sides = std::uint64_t{theme.buttonPadding} * 2;
        const auto inner =
            std::uint64_t{width} > sides ? width - sides : 0;
        const auto advance = antwika::gfx::getScaledGlyphAdvance(
            antwika::gfx::TextScale{
                .face = theme.face, .multiplier = theme.textScale});

        return advance > 0 ? inner / advance : 0;
    }

    std::vector<std::string_view> getWrapText(
        const std::string_view text, const std::size_t columns)
    {
        std::vector<std::string_view> lines;

        if (columns == 0)
        {
            lines.push_back(text);

            return lines;
        }

        std::size_t charIndex = 0;
        auto stop = text.find('\n');

        while (stop != std::string_view::npos)
        {
            wrapRun(text.substr(charIndex, stop - charIndex), columns, lines);
            charIndex = stop + 1;
            stop = text.find('\n', charIndex);
        }

        wrapRun(text.substr(charIndex), columns, lines);

        return lines;
    } // GCOVR_EXCL_LINE

}
