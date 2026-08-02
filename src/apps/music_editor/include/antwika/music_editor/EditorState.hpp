#pragma once

#include <cstddef>
#include <optional>
#include <string>

#include <antwika/ui/ScrollChange.hpp>
#include <antwika/ui/TextEdit.hpp>
#include <antwika/ui/TextFieldSpec.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/music_editor/EditorKeys.hpp"

namespace antwika::music_editor
{

    using antwika::ui::WidgetId;

    /** @brief The id of the one thing the score is typed into. */
    inline constexpr WidgetId kCodeField{1};

    /** @brief The id of the button that starts and stops playback. */
    inline constexpr WidgetId kPlayButton{100};

    /** @brief The id of the button that silences every voice. */
    inline constexpr WidgetId kPanicButton{101};

    /** @brief The id of the box naming which keyboard is being read. */
    inline constexpr WidgetId kLayoutBox{102};

    /**
     * @brief The id the first of that box's options carries.
     *
     * Far from every other id here, since option `n` carries this plus
     * `n` and that whole range has to stay free.
     */
    inline constexpr WidgetId kLayoutOptions{200};

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

        /**
         * @brief Where the selection's other end sits.
         *
         * Absent means nothing is selected, which is what an editor
         * opens with. Absent rather than the caret's own index,
         * because then moving the caret and forgetting this would
         * leave a selection nobody asked for -- and every index is a
         * place a selection can really end.
         */
        std::optional<std::size_t> anchor{};

        /**
         * @brief Which line of the score is drawn at the top of the
         * pane.
         *
         * State for the same reason the caret is: it decides which line
         * a click lands on, so a replay has to reach the same number.
         * antwika::ui works out what it can usefully be and hands that
         * back, which is what keeps the caret in view while typing and
         * what a drag on the scrollbar comes back as.
         */
        std::size_t scroll = 0;

        /**
         * @brief What was last cut or copied.
         *
         * **This editor's own clipboard rather than the window
         * system's**, and that is not a shortcut. What a desktop
         * clipboard holds is not in any recording, so a replay would
         * paste whatever happened to be on the machine replaying it and
         * diverge from the run it is meant to reproduce. Here it is
         * regenerated from the same key presses as everything else.
         */
        std::string clipboard{};

        /** @brief Which keyboard the characters are read off. */
        KeyLayout layout = KeyLayout::Swedish;

        /** @brief Whether that box's list of layouts is showing. */
        bool layoutOpen = false;

        /**
         * @brief Whether a press that landed in the pane is still held.
         *
         * What makes dragging select rather than merely move the caret:
         * a move with this set carries the selection's far end along.
         * A press outside the pane leaves it clear, so a drag that
         * began on a button does not start selecting when it wanders
         * over the text.
         */
        bool dragging = false;

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

    /**
     * @brief Take the line the pane says it is showing.
     *
     * A report naming no area of this editor's is ignored, on the same
     * terms an edit is.
     *
     * @param state What to change.
     * @param scrolled What antwika::ui reported.
     */
    void applyScroll(EditorState &state, const ui::ScrollChange &scrolled);

} // namespace antwika::music_editor
