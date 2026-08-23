#include "TextEditing.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

#include "antwika/ui/Keyboard.hpp"
#include "antwika/ui/TextEdit.hpp"

namespace antwika::ui::detail
{

    namespace
    {
        [[nodiscard]] std::size_t sameColumnIn(
            std::size_t column,
            std::size_t begin,
            std::size_t end) noexcept
        {
            return std::min(begin + column, end);
        }

        [[nodiscard]] std::size_t getLineAbove(const TextEdit &edit) noexcept
        {
            const auto begin = getBeginOfLine(edit.text, edit.cursor);

            if (begin == 0)
            {
                return edit.cursor;
            }

            return sameColumnIn(
                edit.cursor - begin,
                getBeginOfLine(edit.text, begin - 1),
                begin - 1);
        }

        [[nodiscard]] std::size_t getLineBelow(const TextEdit &edit) noexcept
        {
            const auto end = getEndOfLine(edit.text, edit.cursor);

            if (end == edit.text.size())
            {
                return edit.cursor;
            }

            return sameColumnIn(
                edit.cursor - getBeginOfLine(edit.text, edit.cursor),
                end + 1,
                getEndOfLine(edit.text, end + 1));
        }

        [[nodiscard]] bool selects(const TextEdit &edit) noexcept
        {
            return edit.cursor != edit.anchor;
        }

        [[nodiscard]] std::size_t getLowEnd(const TextEdit &edit) noexcept
        {
            return std::min(edit.cursor, edit.anchor);
        }

        [[nodiscard]] std::size_t getHighEnd(const TextEdit &edit) noexcept
        {
            return std::max(edit.cursor, edit.anchor);
        }

        void putCaret(TextEdit &edit, const std::size_t charIndex) noexcept
        {
            edit.cursor = charIndex;
            edit.anchor = charIndex;
        }

        void takeSelection(TextEdit &edit)
        {
            const auto lowIndex = getLowEnd(edit);

            edit.text.erase(lowIndex, getHighEnd(edit) - lowIndex);
            putCaret(edit, lowIndex);
        }

        [[nodiscard]] std::string getSelectedText(const TextEdit &edit)
        {
            const auto lowIndex = getLowEnd(edit);

            return edit.text.substr(lowIndex, getHighEnd(edit) - lowIndex);
        }

        void insert(TextEdit &edit, const char character)
        {
            if (selects(edit))
            {
                takeSelection(edit);
            }

            edit.text.insert(edit.cursor, 1, character);
            putCaret(edit, edit.cursor + 1);
        }
    }

    std::size_t getBeginOfLine(
        const std::string_view text, const std::size_t charIndex) noexcept
    {
        if (charIndex == 0)
        {
            return 0;
        }

        const auto foundIndex = text.rfind('\n', charIndex - 1);

        return foundIndex == std::string_view::npos ? 0 : foundIndex + 1;
    }

    std::size_t getEndOfLine(
        const std::string_view text, const std::size_t charIndex) noexcept
    {
        const auto foundIndex = text.find('\n', charIndex);

        return foundIndex == std::string_view::npos ? text.size() : foundIndex;
    }

    std::optional<TextEdit> editFor(
        const TextEditInput &fieldInput, const Keyboard &keyboard)
    {
        TextEdit edit{
            .fieldWidget = fieldInput.widgetId,
            .text = std::string{fieldInput.text}, // GCOVR_EXCL_LINE
            .cursor = fieldInput.cursor, // GCOVR_EXCL_LINE
            .anchor = fieldInput.anchor}; // GCOVR_EXCL_LINE

        std::size_t nextCharacter = 0;

        for (const auto key : keyboard.keys)
        {
            if (
            key == Key::Character && nextCharacter < keyboard.typedText.size())
            {
                insert(edit, keyboard.typedText[nextCharacter]);
                ++nextCharacter;
            }

            if (key == Key::Backspace && selects(edit))
            {
                takeSelection(edit);
            }
            else if (key == Key::Backspace && edit.cursor > 0)
            {
                edit.text.erase(edit.cursor - 1, 1);
                putCaret(edit, edit.cursor - 1);
            }

            if (key == Key::Delete && selects(edit))
            {
                takeSelection(edit);
            }
            else if (key == Key::Delete && edit.cursor < edit.text.size())
            {
                edit.text.erase(edit.cursor, 1);
                putCaret(edit, edit.cursor);
            }

            if (key == Key::MoveLeft && selects(edit))
            {
                putCaret(edit, getLowEnd(edit));
            }
            else if (key == Key::MoveLeft)
            {
                putCaret(edit, edit.cursor > 0 ? edit.cursor - 1 : 0);
            }

            if (key == Key::MoveRight && selects(edit))
            {
                putCaret(edit, getHighEnd(edit));
            }
            else if (key == Key::MoveRight)
            {
                putCaret(
                    edit, std::min(edit.cursor + 1, edit.text.size()));
            }

            if (key == Key::MoveUp)
            {
                putCaret(edit, getLineAbove(edit));
            }

            if (key == Key::MoveDown)
            {
                putCaret(edit, getLineBelow(edit));
            }

            if (key == Key::MoveLineStart)
            {
                putCaret(edit, getBeginOfLine(edit.text, edit.cursor));
            }

            if (key == Key::MoveLineEnd)
            {
                putCaret(edit, getEndOfLine(edit.text, edit.cursor));
            }

            if (key == Key::SelectLeft && edit.cursor > 0)
            {
                --edit.cursor;
            }

            if (key == Key::SelectRight && edit.cursor < edit.text.size())
            {
                ++edit.cursor;
            }

            if (key == Key::SelectUp)
            {
                edit.cursor = getLineAbove(edit);
            }

            if (key == Key::SelectDown)
            {
                edit.cursor = getLineBelow(edit);
            }

            if (key == Key::SelectLineStart)
            {
                edit.cursor = getBeginOfLine(edit.text, edit.cursor);
            }

            if (key == Key::SelectLineEnd)
            {
                edit.cursor = getEndOfLine(edit.text, edit.cursor);
            }

            if (key == Key::SelectAll)
            {
                edit.anchor = 0;
                edit.cursor = edit.text.size();
            }

            if ((key == Key::Copy || key == Key::Cut) && selects(edit))
            {
                edit.copiedText = getSelectedText(edit);
            }

            if (key == Key::Cut && selects(edit))
            {
                takeSelection(edit);
            }

            if (key == Key::Activate)
            {
                if (fieldInput.multiline)
                {
                    insert(edit, '\n');
                }
                else
                {
                    edit.submitted = true;
                }
            }

            if (key == Key::Cancel)
            {
                edit.cancelled = true;
            }

        }

        const bool changed = std::string_view{edit.text} != fieldInput.text
                             || edit.cursor != fieldInput.cursor
                             || edit.anchor != fieldInput.anchor
                             || !edit.copiedText.empty();

        if (!changed && !edit.submitted && !edit.cancelled)
        {
            return {};
        }

        return edit;
    }

}
