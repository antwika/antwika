#include "TextEditing.hpp"

#include <algorithm>
#include <array>
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

        [[nodiscard]] std::size_t getCharBefore(
            const TextEdit &edit) noexcept
        {
            return edit.cursor > 0 ? edit.cursor - 1 : 0;
        }

        [[nodiscard]] std::size_t getCharAfter(
            const TextEdit &edit) noexcept
        {
            return std::min(edit.cursor + 1, edit.text.size());
        }

        [[nodiscard]] std::size_t getLineBegin(
            const TextEdit &edit) noexcept
        {
            return getBeginOfLine(edit.text, edit.cursor);
        }

        [[nodiscard]] std::size_t getLineEnd(const TextEdit &edit) noexcept
        {
            return getEndOfLine(edit.text, edit.cursor);
        }

        using Motion = std::size_t (*)(const TextEdit &) noexcept;

        struct MotionKey final
        {
            Key key;

            Motion motion;

            Motion selectionMotion;

            bool extendsSelection;
        };

        constexpr std::array<MotionKey, 12> kMotionKeys{{
            {Key::MoveLeft, getCharBefore, getLowEnd, false},
            {Key::MoveRight, getCharAfter, getHighEnd, false},
            {Key::MoveUp, getLineAbove, getLineAbove, false},
            {Key::MoveDown, getLineBelow, getLineBelow, false},
            {Key::MoveLineStart, getLineBegin, getLineBegin, false},
            {Key::MoveLineEnd, getLineEnd, getLineEnd, false},
            {Key::SelectLeft, getCharBefore, getCharBefore, true},
            {Key::SelectRight, getCharAfter, getCharAfter, true},
            {Key::SelectUp, getLineAbove, getLineAbove, true},
            {Key::SelectDown, getLineBelow, getLineBelow, true},
            {Key::SelectLineStart, getLineBegin, getLineBegin, true},
            {Key::SelectLineEnd, getLineEnd, getLineEnd, true},
        }};

        [[nodiscard]] const MotionKey *motionFor(const Key key) noexcept
        {
            for (const auto &motionKey : kMotionKeys)
            {
                if (motionKey.key == key)
                {
                    return &motionKey;
                }
            }

            return nullptr;
        }

        void applyMotion(TextEdit &edit, const MotionKey &motionKey)
        {
            const auto motion =
                !motionKey.extendsSelection && selects(edit)
                    ? motionKey.selectionMotion
                    : motionKey.motion;

            if (motionKey.extendsSelection)
            {
                edit.cursor = motion(edit);
                return;
            }

            putCaret(edit, motion(edit));
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
            if (const auto *motionKey = motionFor(key))
            {
                applyMotion(edit, *motionKey);
                continue;
            }

            switch (key)
            {
            case Key::Character:
                if (nextCharacter < keyboard.typedText.size())
                {
                    insert(edit, keyboard.typedText[nextCharacter]);
                    ++nextCharacter;
                }
                break;
            case Key::Backspace:
                if (selects(edit))
                {
                    takeSelection(edit);
                }
                else if (edit.cursor > 0)
                {
                    edit.text.erase(edit.cursor - 1, 1);
                    putCaret(edit, edit.cursor - 1);
                }
                break;
            case Key::Delete:
                if (selects(edit))
                {
                    takeSelection(edit);
                }
                else if (edit.cursor < edit.text.size())
                {
                    edit.text.erase(edit.cursor, 1);
                    putCaret(edit, edit.cursor);
                }
                break;
            case Key::SelectAll:
                edit.anchor = 0;
                edit.cursor = edit.text.size();
                break;
            case Key::Copy:
                if (selects(edit))
                {
                    edit.copiedText = getSelectedText(edit);
                }
                break;
            case Key::Cut:
                if (selects(edit))
                {
                    edit.copiedText = getSelectedText(edit);
                    takeSelection(edit);
                }
                break;
            case Key::Activate:
                if (fieldInput.multiline)
                {
                    insert(edit, '\n');
                }
                else
                {
                    edit.submitted = true;
                }
                break;
            case Key::Cancel:
                edit.cancelled = true;
                break;
            default:
                break;
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
