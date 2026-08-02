#include "antwika/music_editor/EditorState.hpp"

#include <cstddef>

#include <antwika/ui/TextFieldSpec.hpp>

namespace antwika::music_editor
{

    EditorState openingState()
    {
        EditorState state;

        // Four lines that already make something.
        // Written in the notation the editor is for.
        state.lines[0] = "0 ~ 0 [~ 3]";
        state.lines[1] = "<12 7> ~ 10 ~";
        state.lines[2] = "~ 19 ~ [24 19]";
        state.lines[3] = "0(3,8)";

        return state;

        // The excluded line is the local state's unwind destructor.
        // Nothing between its construction and the return throws.
    } // GCOVR_EXCL_LINE

    void applyEdit(EditorState &state, const ui::TextEdit &edit)
    {
        for (std::size_t track = 0; track < kTrackCount; ++track)
        {
            if (edit.field != fieldFor(track))
            {
                continue;
            }

            state.lines[track] = edit.text;
            state.cursor = edit.cursor;

            return;
        }
    }

    void focusNext(EditorState &state) noexcept
    {
        state.focused = (state.focused + 1) % kTrackCount;
        state.cursor = ui::kCaretAtEnd;
    }

    void focusPrevious(EditorState &state) noexcept
    {
        state.focused =
            (state.focused + kTrackCount - 1) % kTrackCount;

        state.cursor = ui::kCaretAtEnd;
    }

    bool focusWidget(EditorState &state, const WidgetId widget) noexcept
    {
        for (std::size_t track = 0; track < kTrackCount; ++track)
        {
            if (widget != fieldFor(track))
            {
                continue;
            }

            state.focused = track;
            state.cursor = ui::kCaretAtEnd;

            return true;
        }

        return false;
    }

} // namespace antwika::music_editor
