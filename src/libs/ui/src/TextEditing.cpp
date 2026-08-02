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
        // Where the caret goes from a line to the one beside it.
        // The column is kept, or the shorter line's end is taken.
        [[nodiscard]] std::size_t sameColumnIn(
            std::size_t column,
            std::size_t begin,
            std::size_t end) noexcept
        {
            return std::min(begin + column, end);
        }
    } // namespace

    std::size_t beginOfLine(
        const std::string_view text, const std::size_t at) noexcept
    {
        if (at == 0)
        {
            return 0;
        }

        const auto found = text.rfind('\n', at - 1);

        return found == std::string_view::npos ? 0 : found + 1;
    }

    std::size_t endOfLine(
        const std::string_view text, const std::size_t at) noexcept
    {
        const auto found = text.find('\n', at);

        return found == std::string_view::npos ? text.size() : found;
    }

    std::optional<TextEdit> editFor(
        const Editable &field, const Keyboard &keys)
    {
        TextEdit edit{
            .field = field.id,
            .text = std::string{field.text}, // GCOVR_EXCL_LINE
            .cursor = field.cursor}; // GCOVR_EXCL_LINE

        bool moved = false;

        // Which of Keyboard::typed the next Character edge takes.
        std::size_t nextCharacter = 0;

        for (const auto key : keys.keys)
        {
            // A character with no edge to take it is never typed.
            // Nothing would say where in this order it belonged.
            if (key == Key::Character && nextCharacter < keys.typed.size())
            {
                edit.text.insert(
                    edit.cursor, 1, keys.typed[nextCharacter]);

                ++nextCharacter;
                ++edit.cursor;
                moved = true;
            }

            // Backspace at the start has nothing to take.
            if (key == Key::Backspace && edit.cursor > 0)
            {
                edit.text.erase(edit.cursor - 1, 1);
                --edit.cursor;
                moved = true;
            }

            if (key == Key::MoveLeft && edit.cursor > 0)
            {
                --edit.cursor;
                moved = true;
            }

            if (key == Key::MoveRight && edit.cursor < edit.text.size())
            {
                ++edit.cursor;
                moved = true;
            }

            if (key == Key::MoveUp)
            {
                const auto begin = beginOfLine(edit.text, edit.cursor);

                // The first line has nothing above it.
                if (begin > 0)
                {
                    edit.cursor = sameColumnIn(
                        edit.cursor - begin,
                        beginOfLine(edit.text, begin - 1),
                        begin - 1);

                    moved = true;
                }
            }

            if (key == Key::MoveDown)
            {
                const auto end = endOfLine(edit.text, edit.cursor);

                // The last line has nothing below it.
                if (end < edit.text.size())
                {
                    const auto column =
                        edit.cursor - beginOfLine(edit.text, edit.cursor);

                    edit.cursor = sameColumnIn(
                        column, end + 1, endOfLine(edit.text, end + 1));

                    moved = true;
                }
            }

            // The same edge resolve() activates the widget on.
            // In an area it is what a line break is written with.
            if (key == Key::Activate)
            {
                if (field.multiline)
                {
                    edit.text.insert(edit.cursor, 1, '\n');
                    ++edit.cursor;
                    moved = true;
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

            // A focus key is focus's alone, and moves no caret.
        }

        if (!moved && !edit.submitted && !edit.cancelled)
        {
            return {};
        }

        return edit;
    }

} // namespace antwika::ui::detail
