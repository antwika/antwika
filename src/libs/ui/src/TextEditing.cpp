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

        // Where the caret lands one line up.
        // Where it already is when there is no line above it.
        [[nodiscard]] std::size_t lineAbove(const TextEdit &edit) noexcept
        {
            const auto begin = beginOfLine(edit.text, edit.cursor);

            if (begin == 0)
            {
                return edit.cursor;
            }

            return sameColumnIn(
                edit.cursor - begin,
                beginOfLine(edit.text, begin - 1),
                begin - 1);
        }

        [[nodiscard]] std::size_t lineBelow(const TextEdit &edit) noexcept
        {
            const auto end = endOfLine(edit.text, edit.cursor);

            if (end == edit.text.size())
            {
                return edit.cursor;
            }

            return sameColumnIn(
                edit.cursor - beginOfLine(edit.text, edit.cursor),
                end + 1,
                endOfLine(edit.text, end + 1));
        }

        [[nodiscard]] bool selects(const TextEdit &edit) noexcept
        {
            return edit.cursor != edit.anchor;
        }

        [[nodiscard]] std::size_t lowEnd(const TextEdit &edit) noexcept
        {
            return std::min(edit.cursor, edit.anchor);
        }

        [[nodiscard]] std::size_t highEnd(const TextEdit &edit) noexcept
        {
            return std::max(edit.cursor, edit.anchor);
        }

        // Both ends at once, since a caret is a selection of nothing.
        // Which is what every move below wants and no select does.
        void putCaret(TextEdit &edit, const std::size_t at) noexcept
        {
            edit.cursor = at;
            edit.anchor = at;
        }

        // What every key that writes does to a selection first.
        // Which is to take the whole of it.
        void takeSelection(TextEdit &edit)
        {
            const auto low = lowEnd(edit);

            edit.text.erase(low, highEnd(edit) - low);
            putCaret(edit, low);
        }

        [[nodiscard]] std::string selectedText(const TextEdit &edit)
        {
            const auto low = lowEnd(edit);

            return edit.text.substr(low, highEnd(edit) - low);
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
            .cursor = field.cursor, // GCOVR_EXCL_LINE
            .anchor = field.anchor}; // GCOVR_EXCL_LINE

        // Which of Keyboard::typed the next Character edge takes.
        std::size_t nextCharacter = 0;

        for (const auto key : keys.keys)
        {
            // A character with no edge to take it is never typed.
            // Nothing would say where in this order it belonged.
            if (key == Key::Character && nextCharacter < keys.typed.size())
            {
                insert(edit, keys.typed[nextCharacter]);
                ++nextCharacter;
            }

            // Both of these take a whole selection when there is one.
            // Which is most of what having a selection is for.
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

            // A plain move off a selection lands on the end it is heading for.
            // Rather than one character past the caret.
            // Which is the one thing every editor agrees about here.
            if (key == Key::MoveLeft && selects(edit))
            {
                putCaret(edit, lowEnd(edit));
            }
            else if (key == Key::MoveLeft)
            {
                putCaret(edit, edit.cursor > 0 ? edit.cursor - 1 : 0);
            }

            if (key == Key::MoveRight && selects(edit))
            {
                putCaret(edit, highEnd(edit));
            }
            else if (key == Key::MoveRight)
            {
                putCaret(
                    edit, std::min(edit.cursor + 1, edit.text.size()));
            }

            if (key == Key::MoveUp)
            {
                putCaret(edit, lineAbove(edit));
            }

            if (key == Key::MoveDown)
            {
                putCaret(edit, lineBelow(edit));
            }

            // Each of these leaves the anchor exactly where it was.
            // That is the whole of what makes them select.
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
                edit.cursor = lineAbove(edit);
            }

            if (key == Key::SelectDown)
            {
                edit.cursor = lineBelow(edit);
            }

            // Reported and never kept, since nothing here outlives the frame.
            // A Cut takes with it what it just reported.
            if ((key == Key::Copy || key == Key::Cut) && selects(edit))
            {
                edit.copied = selectedText(edit);
            }

            if (key == Key::Cut && selects(edit))
            {
                takeSelection(edit);
            }

            // The same edge resolve() activates the widget on.
            // In an area it is what a line break is written with.
            if (key == Key::Activate)
            {
                if (field.multiline)
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

            // A focus key is focus's alone, and moves no caret.
        }

        // Asked of the answer rather than tracked as it was built.
        // A pair of keys putting the caret back changed nothing.
        // Every branch above would have to know that for a flag to say so.
        const bool changed = std::string_view{edit.text} != field.text
                             || edit.cursor != field.cursor
                             || edit.anchor != field.anchor
                             || !edit.copied.empty();

        if (!changed && !edit.submitted && !edit.cancelled)
        {
            return {};
        }

        return edit;
    }

} // namespace antwika::ui::detail
