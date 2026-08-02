#pragma once

#include <cstddef>
#include <string>

#include <antwika/ui/TextEdit.hpp>
#include <antwika/ui/TextFieldSpec.hpp>
#include <antwika/ui/WidgetId.hpp>

namespace antwika::music_editor
{

    using antwika::ui::WidgetId;

    /** @brief The id of the one thing the score is typed into. */
    inline constexpr WidgetId kCodeField{1};

    /** @brief The id of the button that starts and stops playback. */
    inline constexpr WidgetId kPlayButton{100};

    /** @brief The id of the button that silences every voice. */
    inline constexpr WidgetId kPanicButton{101};

    /**
     * @brief Everything the editor holds between ticks.
     *
     * **All of it is simulation state**, and every bit of it is derived
     * from key and pointer edges the recording already carries -- so
     * this app defines no event of its own, and a replay retypes the
     * session rather than replaying its text.
     *
     * What is deliberately *not* here is what the document parses into,
     * the sequencer's position, and anything a voice is made of.
     * Those are regenerated from the text, which is why editing it
     * changes what is playing without anything being told to reload.
     */
    struct EditorState
    {
        /** @brief The whole score, as one document of many lines. */
        std::string source{};

        /** @brief Where the caret sits, as an index into it. */
        std::size_t cursor = ui::kCaretAtEnd;

        /** @brief Whether the sequencer's clock is standing still. */
        bool paused = false;

        /**
         * @brief Compare two states.
         * @param other The state to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(const EditorState &other) const
            = default;
    };

    /**
     * @brief Get the state an empty editor opens with.
     * @return The starting state.
     */
    [[nodiscard]] EditorState openingState();

    /**
     * @brief Apply what a frame's typing did to the document.
     *
     * An edit naming no widget of this editor's is ignored, which is
     * what makes it safe to hand every frame's edit straight here.
     *
     * @param state What to change.
     * @param edit What antwika::ui reported.
     */
    void applyEdit(EditorState &state, const ui::TextEdit &edit);

} // namespace antwika::music_editor
