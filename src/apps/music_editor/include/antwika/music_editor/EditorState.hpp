#pragma once

#include <array>
#include <cstddef>
#include <string>

#include <antwika/ui/TextEdit.hpp>
#include <antwika/ui/TextFieldSpec.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/music_editor/TrackPreset.hpp"

namespace antwika::music_editor
{

    using antwika::ui::WidgetId;

    /** @brief The id of the field a track is typed into. */
    [[nodiscard]] constexpr WidgetId fieldFor(std::size_t track) noexcept
    {
        // One past zero, so the first field is not kNoWidget.
        return WidgetId{static_cast<std::uint64_t>(track) + 1};
    }

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
     * What is deliberately *not* here is the pattern the lines parse
     * into, the sequencer's position, and anything a voice is made of.
     * Those are regenerated from these lines, which is why editing one
     * changes what is playing without anything being told to reload.
     */
    struct EditorState
    {
        /** @brief One line of mini-notation per track. */
        std::array<std::string, kTrackCount> lines{};

        /** @brief Which line the typing goes into. */
        std::size_t focused = 0;

        /** @brief Where the caret sits in the focused line. */
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
     * @brief Get the lines an empty editor opens with.
     *
     * Something that plays, because an editor that opens silent gives a
     * newcomer nothing to change.
     *
     * @return The starting state.
     */
    [[nodiscard]] EditorState openingState();

    /**
     * @brief Apply what a frame's typing did to a field.
     *
     * An edit naming no field of this editor's is ignored, which is what
     * makes it safe to hand every frame's edit straight here.
     *
     * @param state What to change.
     * @param edit What antwika::ui reported.
     */
    void applyEdit(EditorState &state, const ui::TextEdit &edit);

    /**
     * @brief Move the typing to the next line, wrapping at the end.
     * @param state What to change.
     */
    void focusNext(EditorState &state) noexcept;

    /**
     * @brief Move the typing to the previous line, wrapping at the top.
     * @param state What to change.
     */
    void focusPrevious(EditorState &state) noexcept;

    /**
     * @brief Give the typing to a field a pointer activated.
     * @param state What to change.
     * @param widget What was activated, which may name no field.
     * @return True when the focus moved.
     */
    bool focusWidget(EditorState &state, WidgetId widget) noexcept;

} // namespace antwika::music_editor
