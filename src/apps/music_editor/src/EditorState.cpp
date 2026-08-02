#include "antwika/music_editor/EditorState.hpp"

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
    }

} // namespace antwika::music_editor
