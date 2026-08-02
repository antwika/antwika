#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/ui/ScrollChange.hpp>
#include <antwika/ui/TextAreaSpec.hpp>
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

    /** @brief The id of the menu box at the top of the screen. */
    inline constexpr WidgetId kMenuBox{103};

    /** @brief The id of the box naming how fast the clock runs. */
    inline constexpr WidgetId kSpeedBox{104};

    /** @brief The id of the save modal's name field. */
    inline constexpr WidgetId kSaveNameField{110};

    /** @brief The id of the save modal's confirming button. */
    inline constexpr WidgetId kSaveConfirm{111};

    /** @brief The id of the button that closes either modal unsaved. */
    inline constexpr WidgetId kModalCancel{112};

    /**
     * @brief The id the first of that box's options carries.
     *
     * Far from every other id here, since option `n` carries this plus
     * `n` and that whole range has to stay free.
     */
    inline constexpr WidgetId kLayoutOptions{200};

    /** @brief The menu's first option's id, on kLayoutOptions' terms. */
    inline constexpr WidgetId kMenuOptions{300};

    /** @brief The id the load modal's first score carries, likewise. */
    inline constexpr WidgetId kLoadOptions{400};

    /** @brief The speed box's first option's id, likewise. */
    inline constexpr WidgetId kSpeedOptions{500};

    /**
     * @brief One speed the playback can be asked to run at.
     */
    struct SpeedChoice
    {
        /** @brief What the box shows for it. */
        std::string_view label;

        /** @brief The multiplier's top: 2 is twice as fast. */
        std::int64_t numerator;

        /** @brief The multiplier's bottom: 2 is half as fast. */
        std::int64_t denominator;
    };

    /**
     * @brief The speeds there are to choose, slowest first.
     *
     * One table for the labels the scene shows and the multipliers the
     * sink applies, so the two can never disagree about what a choice
     * means.  Fractions rather than floats for pattern's reason: a
     * tempo is exact arithmetic a replay has to reproduce.
     */
    inline constexpr std::array<SpeedChoice, 5> kSpeeds{
        SpeedChoice{.label = "0.25x", .numerator = 1, .denominator = 4},
        SpeedChoice{.label = "0.5x", .numerator = 1, .denominator = 2},
        SpeedChoice{.label = "1x", .numerator = 1, .denominator = 1},
        SpeedChoice{.label = "2x", .numerator = 2, .denominator = 1},
        SpeedChoice{.label = "4x", .numerator = 4, .denominator = 1}};

    /** @brief Which of kSpeeds a fresh editor runs at. */
    inline constexpr std::size_t kNormalSpeed = 2;

    /**
     * @brief Get the id the load box's nth score button carries.
     * @param at Which score, as an index into EditorState::scores.
     * @return Its id, kLoadOptions plus the index.
     */
    [[nodiscard]] constexpr WidgetId loadOption(
        const std::size_t at) noexcept
    {
        return WidgetId{
            static_cast<std::uint64_t>(kLoadOptions) + at};
    }

    /**
     * @brief Which box over the editor has the input, if any.
     */
    enum class Modal : std::uint8_t
    {
        /** @brief None: the editor itself is what is typed into. */
        None = 0,

        /** @brief The save box, asking what to call the score. */
        Save,

        /** @brief The load box, listing what there is to open. */
        Load,
    };

    /**
     * @brief Everything the editor holds between ticks.
     *
     * **All of it is simulation state**, and nearly every bit of it is
     * derived from key and pointer edges the recording already carries
     * -- so a replay retypes the session rather than replaying its
     * text.  The exceptions carry their own reasons: a paste's
     * characters arrive as events::kPaste (see clipboard below), and
     * the score list is read once at startup (see scores below).
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
         * Simulation state, regenerated from the same key presses as
         * everything else -- and the window system's clipboard is fed
         * *from* it, by a live run's sink, as an outward write no tick
         * ever reads back. Reading goes the other way entirely: a
         * paste's characters arrive as an events::kPaste the recording
         * carries, read upstream of the recorder by PasteSource, so a
         * replay pastes what the run pasted whatever the replaying
         * machine's clipboard happens to hold.
         */
        std::string clipboard{};

        /** @brief Which keyboard the characters are read off. */
        KeyLayout layout = KeyLayout::Swedish;

        /** @brief Whether that box's list of layouts is showing. */
        bool layoutOpen = false;

        /**
         * @brief Where a still-held press landed in the pane, if one.
         *
         * What makes dragging select rather than merely move the caret,
         * and what keeps the pane's two halves apart: a drag that
         * began in the text never reaches the scrollbar, and one that
         * began on the scrollbar never selects.  Armed from
         * Interactions::areaPress and cleared on the release, so a
         * drag that began on a button selects nothing when it wanders
         * over the text.
         */
        ui::DragHome dragging = ui::DragHome::None;

        /** @brief Whether the sequencer's clock is standing still. */
        bool paused = false;

        /** @brief Whether the menu's list of commands is showing. */
        bool menuOpen = false;

        /** @brief Which of kSpeeds the musical clock runs at. */
        std::size_t speed = kNormalSpeed;

        /** @brief Whether that box's list of speeds is showing. */
        bool speedOpen = false;

        /** @brief Which box is over the editor, taking the input. */
        Modal modal = Modal::None;

        /** @brief What the save box's name field holds. */
        std::string fileName{};

        /** @brief Where that field's caret sits. */
        std::size_t fileCursor = ui::kCaretAtEnd;

        /**
         * @brief What a modal has to say about the last save or load.
         *
         * A refusal mostly -- a name of nothing, a disk that would not
         * take it -- shown inside the box and cleared when one closes.
         */
        std::string notice{};

        /**
         * @brief The scores there are to load, sorted by name.
         *
         * **Simulation state on the same terms apps/game's save list
         * is**: which button a click lands on is a function of it, so
         * it is read from the directory once at startup, upstream of
         * the loop, and a save made here is added to this copy rather
         * than the directory being read again.  See listScores().
         */
        std::vector<std::string> scores{};

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
     * @brief Put a saved score's name into the list, in order.
     *
     * A sorted insert rather than a re-listing, because a directory
     * read inside the tick path would not replay; saving over a name
     * the list already has changes nothing here.
     *
     * @param state Whose list to grow.
     * @param name What the score was saved as.
     */
    void addScore(EditorState &state, const std::string &name);

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
