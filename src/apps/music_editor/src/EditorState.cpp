#include "antwika/music_editor/EditorState.hpp"

#include <algorithm>

#include <antwika/ui/TextEdit.hpp>

#include "antwika/music_editor/Score.hpp"

namespace antwika::music_editor
{

    EditorState openingState()
    {
        EditorState state;

        state.source = openingSource();

        return state;

        // The excluded line is the local state's unwind destructor.
        // Nothing between its construction and the return throws.
    } // GCOVR_EXCL_LINE

    void applyEdit(EditorState &state, const ui::TextEdit &edit)
    {
        if (edit.field != kCodeField)
        {
            return;
        }

        state.source = edit.text;
        state.cursor = edit.cursor;
        state.anchor = edit.anchor;

        // A copy with nothing selected reports nothing.
        // It must not empty what was copied before it.
        if (!edit.copied.empty())
        {
            state.clipboard = edit.copied;
        }
    }

    void addScore(EditorState &state, const std::string &name)
    {
        const auto at = std::lower_bound(
            state.scores.begin(), state.scores.end(), name);

        if (at == state.scores.end() || *at != name)
        {
            state.scores.insert(at, name);
        }
    }

    void applyScroll(EditorState &state, const ui::ScrollChange &scrolled)
    {
        if (scrolled.area != kCodeField)
        {
            return;
        }

        state.scroll = scrolled.line;
    }

} // namespace antwika::music_editor
