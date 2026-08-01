#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace antwika::i18n
{

    /**
     * @brief A symbolic name for one translatable string.
     *
     * A catalogue is keyed by this rather than by the English text, so a
     * locale that is missing a string produces a lookup that *reports* a
     * miss.
     * Keying by the English string would make a missing translation fall
     * through to English prose embedded in the calling code, which is
     * indistinguishable from a translation that happens to read the same
     * in both languages.
     *
     * **Every id an application shows lives here, in one enumeration.**
     * That is not a convenience: a catalogue is complete only against a
     * list of everything there is, so an id declared beside the
     * application showing it could never be checked for.
     * Growing this is therefore a library edit, and adding an
     * enumerator without adding its text to both catalogues is a red
     * build rather than an English label in a Swedish window.
     * The names read `<Application><Thing>`, since one flat set has to
     * say which corner of which program a string belongs to.
     */
    enum class MessageId : std::uint16_t
    {
        /**
         * @brief Main menu: start a new session.
         */
        MenuPlayGame,

        /**
         * @brief Main menu: open a recorded session.
         */
        MenuLoadReplay,

        /**
         * @brief Main menu: write the current session out.
         */
        MenuSaveReplay,

        /**
         * @brief Main menu: go back to the session already running.
         */
        MenuResumeGame,

        /**
         * @brief Main menu: the label above the language choices.
         */
        MenuLanguage,

        /**
         * @brief The name of the English language.
         */
        LanguageEnglish,

        /**
         * @brief The name of the Swedish language.
         */
        LanguageSwedish,

        /**
         * @brief Toolbar: zoom one level in.
         */
        ToolbarZoomIn,

        /**
         * @brief Toolbar: zoom one level out.
         */
        ToolbarZoomOut,

        /**
         * @brief Toolbar: put the camera back where the run started.
         */
        ToolbarResetView,

        /**
         * @brief Toolbar: the current zoom level, with `{0}` as the
         *        level.
         */
        ToolbarZoomLevel,

        /**
         * @brief Atlas editor: the tool that puts a colour down.
         */
        AtlasToolPaint,

        /**
         * @brief Atlas editor: the tool that clears a pixel.
         */
        AtlasToolErase,

        /**
         * @brief Atlas editor: the tool that takes a colour.
         */
        AtlasToolPick,

        /**
         * @brief Atlas editor: put the whole sheet back in the middle.
         */
        AtlasResetView,

        /**
         * @brief Atlas editor: show or hide the slot grid.
         */
        AtlasGrid,

        /**
         * @brief Atlas editor: read a sheet back in.
         */
        AtlasLoad,

        /**
         * @brief Atlas editor: write the sheet out.
         */
        AtlasSave,

        /**
         * @brief Atlas editor: the pointer is on no pixel of the sheet.
         */
        AtlasPixelUnknown,

        /**
         * @brief Atlas editor: which pixel the pointer is on, `{0}` across
         *        and `{1}` down.
         */
        AtlasPixelAt,

        /**
         * @brief Atlas editor: which slot that pixel falls in, `{0}`.
         */
        AtlasSlot,

        /**
         * @brief Atlas editor: the sheet has changed since it was written.
         */
        AtlasUnsaved,

        /**
         * @brief Atlas editor: the sheet was written to `{0}`.
         */
        AtlasSaved,

        /**
         * @brief Atlas editor: the sheet was not written, because `{0}`.
         *        The reason is a diagnostic and is never translated.
         */
        AtlasSaveFailed,

        /**
         * @brief Atlas editor: no sheet was named for a load to read.
         */
        AtlasNothingToLoad,

        /**
         * @brief Atlas editor: a sheet was read in.
         */
        AtlasLoaded,

        /**
         * @brief Atlas editor: no sheet was read, because `{0}`.
         *        The reason is a diagnostic and is never translated.
         */
        AtlasLoadFailed,

        /**
         * @brief Companion: how hungry it is, `{0}` of its maximum.
         */
        CompanionHunger,

        /**
         * @brief Companion: how happy it is, `{0}` of its maximum.
         */
        CompanionHappy,

        /**
         * @brief Companion: awake and wanting nothing.
         */
        CompanionAwake,

        /**
         * @brief Companion: awake and wanting a meal.
         */
        CompanionAwakeHungry,

        /**
         * @brief Companion: asleep and undisturbed.
         */
        CompanionAsleep,

        /**
         * @brief Companion: asleep and woken by a tap.
         */
        CompanionAsleepWoken,

        /**
         * @brief Companion: it has perished.
         */
        CompanionGone,

        /**
         * @brief Companion: the button starting a new one.
         */
        CompanionNewPet,

        /**
         * @brief Companion: the label on the prop that feeds it.
         */
        CompanionPropFeed,

        /**
         * @brief Companion: the label on the prop that plays with it.
         */
        CompanionPropPlay,

        /**
         * @brief Companion: the label on the prop that puts it to bed.
         */
        CompanionPropSleep,

        /**
         * @brief Companion says: idle chatter.
         */
        CompanionSayHello,

        /**
         * @brief Companion says: idle chatter.
         */
        CompanionSayBored,

        /**
         * @brief Companion says: idle chatter.
         */
        CompanionSayNiceDay,

        /**
         * @brief Companion says: idle chatter.
         */
        CompanionSayLaLaLa,

        /**
         * @brief Companion says: it is hungry.
         */
        CompanionSayFeedMe,

        /**
         * @brief Companion says: it has been fed.
         */
        CompanionSayYumYum,

        /**
         * @brief Companion says: it was fed and did not want.
         */
        CompanionSayFull,

        /**
         * @brief Companion says: it was woken.
         */
        CompanionSayShhh,

        /**
         * @brief Companion says: it is sleeping quietly.
         */
        CompanionSayZzz,

        /**
         * @brief Companion says: it wants to be played with.
         */
        CompanionSayPlay,

        /**
         * @brief Companion says: it is being played with.
         */
        CompanionSayWheee,

        /**
         * @brief Companion says: it has no energy left to play.
         */
        CompanionSayTooTired,

        /**
         * @brief Companion says: it was sent to bed wide awake.
         */
        CompanionSayNotSleepy,

        /**
         * @brief Companion says: it has become tired enough for bed.
         */
        CompanionSayYawn,

        /**
         * @brief Companion says: a press landed on no prop.
         */
        CompanionSayPoked,

        /**
         * @brief Companion life stage: an egg.
         */
        CompanionStageEgg,

        /**
         * @brief Companion life stage: a child.
         */
        CompanionStageChild,

        /**
         * @brief Companion life stage: a teenager.
         */
        CompanionStageTeen,

        /**
         * @brief Companion life stage: an adult.
         */
        CompanionStageAdult,

        /**
         * @brief Companion life stage: an elder.
         */
        CompanionStageElder,

        /**
         * @brief Companion day mood: hunger comes on faster.
         */
        CompanionMoodHungry,

        /**
         * @brief Companion day mood: boredom comes on faster.
         */
        CompanionMoodRestless,

        /**
         * @brief Companion day mood: energy drains faster.
         */
        CompanionMoodHeavy,

        /**
         * @brief Companion: the day, `{0}`, its stage `{1}` and its mood `{2}`.
         */
        CompanionDay,

        /**
         * @brief Companion: the generation `{0}` and the best `{1}`.
         */
        CompanionLineage,

        /**
         * @brief Sudoku: the heading over the puzzle as given.
         */
        SudokuInput,

        /**
         * @brief Sudoku: the heading over the finished grid.
         */
        SudokuSolved,

        /**
         * @brief Sudoku: the puzzle has no solution at all.
         */
        SudokuNoSolution,

        /**
         * @brief Sudoku: the solver gave up before deciding.
         */
        SudokuLimitExceeded,

        /**
         * @brief UI demo: the heading over every page.
         */
        UiDemoTitle,

        /**
         * @brief UI demo: the page picker with nothing chosen yet.
         */
        UiDemoPickPage,

        /**
         * @brief UI demo: the page of labels.
         */
        UiDemoPageLabels,

        /**
         * @brief UI demo: the page of buttons.
         */
        UiDemoPageButtons,

        /**
         * @brief UI demo: the page of nested containers.
         */
        UiDemoPageLayout,

        /**
         * @brief UI demo: the page holding a text field.
         */
        UiDemoPageTextField,

        /**
         * @brief UI demo: the page holding a second list.
         */
        UiDemoPageDropdown,

        /**
         * @brief UI demo: the page walked with the keyboard.
         */
        UiDemoPageFocus,

        /**
         * @brief UI demo: the page of theme colours.
         */
        UiDemoPageTheme,

        /**
         * @brief UI demo: the page reading a layout back.
         */
        UiDemoPageRects,

        /**
         * @brief UI demo: the page with less room than its children want.
         */
        UiDemoPageShrink,

        /**
         * @brief UI demo: what a plain label is.
         */
        UiDemoLabelsLine,

        /**
         * @brief UI demo: what a muted label is for.
         */
        UiDemoLabelsMuted,

        /**
         * @brief UI demo: a label takes a colour.
         */
        UiDemoLabelsOwnInk,

        /**
         * @brief UI demo: the label left of a growing spacer.
         */
        UiDemoSpacerLeft,

        /**
         * @brief UI demo: the label right of a growing spacer.
         */
        UiDemoSpacerRight,

        /**
         * @brief UI demo: when a button activates.
         */
        UiDemoButtonsPress,

        /**
         * @brief UI demo: the button counting a press.
         */
        UiDemoButtonCount,

        /**
         * @brief UI demo: the button clearing that count.
         */
        UiDemoButtonReset,

        /**
         * @brief UI demo: how many presses have been counted, `{0}`.
         */
        UiDemoPressedCount,

        /**
         * @brief UI demo: an appearance the caller decided.
         */
        UiDemoButtonsForced,

        /**
         * @brief UI demo: a button forced to look idle.
         */
        UiDemoButtonIdle,

        /**
         * @brief UI demo: a button forced to look hovered.
         */
        UiDemoButtonHovered,

        /**
         * @brief UI demo: a button forced to look pressed.
         */
        UiDemoButtonPressed,

        /**
         * @brief UI demo: a button with no id, which nothing can hit.
         */
        UiDemoButtonUnnamed,

        /**
         * @brief UI demo: the three widths a button takes.
         */
        UiDemoButtonsWidths,

        /**
         * @brief UI demo: a button as wide as its own label.
         */
        UiDemoButtonFit,

        /**
         * @brief UI demo: a button of a stated width.
         */
        UiDemoButtonFixed,

        /**
         * @brief UI demo: a button taking what room is left.
         */
        UiDemoButtonGrow,

        /**
         * @brief UI demo: how deep a layout may nest.
         */
        UiDemoLayoutNest,

        /**
         * @brief UI demo: aligned to the start of the axis.
         */
        UiDemoAlignStart,

        /**
         * @brief UI demo: aligned to the middle of the axis.
         */
        UiDemoAlignCenter,

        /**
         * @brief UI demo: aligned to the end of the axis.
         */
        UiDemoAlignEnd,

        /**
         * @brief UI demo: which axis an alignment is across.
         */
        UiDemoAcrossAxis,

        /**
         * @brief UI demo: what a panel is.
         */
        UiDemoPanelIsColumn,

        /**
         * @brief UI demo: what else a panel is.
         */
        UiDemoPanelInset,

        /**
         * @brief UI demo: who owns a field's characters.
         */
        UiDemoFieldOwned,

        /**
         * @brief UI demo: an empty field's prompt.
         */
        UiDemoFieldPlaceholder,

        /**
         * @brief UI demo: what the two keys do to a field.
         */
        UiDemoFieldKeys,

        /**
         * @brief UI demo: what the field holds right now, `{0}`.
         */
        UiDemoFieldHolding,

        /**
         * @brief UI demo: who owns a list's open flag.
         */
        UiDemoListOpenBit,

        /**
         * @brief UI demo: the accent list with nothing chosen.
         */
        UiDemoNoneChosen,

        /**
         * @brief UI demo: where an open list is drawn.
         */
        UiDemoListOverlay,

        /**
         * @brief UI demo: the first accent colour.
         */
        UiDemoAccentAmber,

        /**
         * @brief UI demo: the second accent colour.
         */
        UiDemoAccentMint,

        /**
         * @brief UI demo: the third accent colour.
         */
        UiDemoAccentRose,

        /**
         * @brief UI demo: the keys that walk a row of buttons.
         */
        UiDemoFocusKeys,

        /**
         * @brief UI demo: the first button of the focus row.
         */
        UiDemoButtonFirst,

        /**
         * @brief UI demo: the second button of the focus row.
         */
        UiDemoButtonSecond,

        /**
         * @brief UI demo: the third button of the focus row.
         */
        UiDemoButtonThird,

        /**
         * @brief UI demo: what the focus ring is made of.
         */
        UiDemoFocusRingFills,

        /**
         * @brief UI demo: which widget has focus, `{0}` being its id.
         */
        UiDemoFocusedId,

        /**
         * @brief UI demo: what a ui::Theme decides on a widget's behalf.
         */
        UiDemoThemeColours,

        /**
         * @brief UI demo: what Frame::rects answers.
         */
        UiDemoRectsSays,

        /**
         * @brief UI demo: the row whose rectangle is read.
         */
        UiDemoRowIsNamed,

        /**
         * @brief UI demo: the bar drawn from that rectangle.
         */
        UiDemoBarFromRect,

        /**
         * @brief UI demo: an id this frame never declared.
         */
        UiDemoUndeclaredId,

        /**
         * @brief UI demo: what too little room does to children.
         */
        UiDemoShrinkProportion,

        /**
         * @brief UI demo: the first of two oversized buttons.
         */
        UiDemoTooWide,

        /**
         * @brief UI demo: the second of two oversized ones.
         */
        UiDemoAlsoTooWide,

        /**
         * @brief UI demo: there is no clipping, first of two lines.
         */
        UiDemoNoClipping,

        /**
         * @brief UI demo: there is no clipping, second of two lines.
         */
        UiDemoLayoutsJob,

        /**
         * @brief UI demo: which page was just chosen, `{0}`.
         */
        UiDemoShowing,

        /**
         * @brief UI demo: which accent was just chosen, `{0}`.
         */
        UiDemoAccentChosen,

        /**
         * @brief UI demo: the field was submitted holding `{0}`.
         */
        UiDemoSubmitted,

        /**
         * @brief UI demo: the field was given up on.
         */
        UiDemoCancelled,

        /**
         * @brief UI demo: a widget with no other answer was pressed, `{0}`
         *        being its id.
         */
        UiDemoPressedWidget,
    };

    /**
     * @brief How many ids a complete catalogue carries.
     */
    inline constexpr std::size_t kMessageCount{134};

    /**
     * @brief Every id, in declaration order.
     *
     * This is what makes the symbolic key pay for itself: a test can walk
     * the whole set and assert that every locale answers for all of it,
     * which is not expressible when the key *is* the English text.
     */
    inline constexpr std::array<MessageId, kMessageCount> kAllMessageIds{
        MessageId::MenuPlayGame,
        MessageId::MenuLoadReplay,
        MessageId::MenuSaveReplay,
        MessageId::MenuResumeGame,
        MessageId::MenuLanguage,
        MessageId::LanguageEnglish,
        MessageId::LanguageSwedish,
        MessageId::ToolbarZoomIn,
        MessageId::ToolbarZoomOut,
        MessageId::ToolbarResetView,
        MessageId::ToolbarZoomLevel,
        MessageId::AtlasToolPaint,
        MessageId::AtlasToolErase,
        MessageId::AtlasToolPick,
        MessageId::AtlasResetView,
        MessageId::AtlasGrid,
        MessageId::AtlasLoad,
        MessageId::AtlasSave,
        MessageId::AtlasPixelUnknown,
        MessageId::AtlasPixelAt,
        MessageId::AtlasSlot,
        MessageId::AtlasUnsaved,
        MessageId::AtlasSaved,
        MessageId::AtlasSaveFailed,
        MessageId::AtlasNothingToLoad,
        MessageId::AtlasLoaded,
        MessageId::AtlasLoadFailed,
        MessageId::CompanionHunger,
        MessageId::CompanionHappy,
        MessageId::CompanionAwake,
        MessageId::CompanionAwakeHungry,
        MessageId::CompanionAsleep,
        MessageId::CompanionAsleepWoken,
        MessageId::CompanionGone,
        MessageId::CompanionNewPet,
        MessageId::CompanionPropFeed,
        MessageId::CompanionPropPlay,
        MessageId::CompanionPropSleep,
        MessageId::CompanionSayHello,
        MessageId::CompanionSayBored,
        MessageId::CompanionSayNiceDay,
        MessageId::CompanionSayLaLaLa,
        MessageId::CompanionSayFeedMe,
        MessageId::CompanionSayYumYum,
        MessageId::CompanionSayFull,
        MessageId::CompanionSayShhh,
        MessageId::CompanionSayZzz,
        MessageId::CompanionSayPlay,
        MessageId::CompanionSayWheee,
        MessageId::CompanionSayTooTired,
        MessageId::CompanionSayNotSleepy,
        MessageId::CompanionSayYawn,
        MessageId::CompanionSayPoked,
        MessageId::CompanionStageEgg,
        MessageId::CompanionStageChild,
        MessageId::CompanionStageTeen,
        MessageId::CompanionStageAdult,
        MessageId::CompanionStageElder,
        MessageId::CompanionMoodHungry,
        MessageId::CompanionMoodRestless,
        MessageId::CompanionMoodHeavy,
        MessageId::CompanionDay,
        MessageId::CompanionLineage,
        MessageId::SudokuInput,
        MessageId::SudokuSolved,
        MessageId::SudokuNoSolution,
        MessageId::SudokuLimitExceeded,
        MessageId::UiDemoTitle,
        MessageId::UiDemoPickPage,
        MessageId::UiDemoPageLabels,
        MessageId::UiDemoPageButtons,
        MessageId::UiDemoPageLayout,
        MessageId::UiDemoPageTextField,
        MessageId::UiDemoPageDropdown,
        MessageId::UiDemoPageFocus,
        MessageId::UiDemoPageTheme,
        MessageId::UiDemoPageRects,
        MessageId::UiDemoPageShrink,
        MessageId::UiDemoLabelsLine,
        MessageId::UiDemoLabelsMuted,
        MessageId::UiDemoLabelsOwnInk,
        MessageId::UiDemoSpacerLeft,
        MessageId::UiDemoSpacerRight,
        MessageId::UiDemoButtonsPress,
        MessageId::UiDemoButtonCount,
        MessageId::UiDemoButtonReset,
        MessageId::UiDemoPressedCount,
        MessageId::UiDemoButtonsForced,
        MessageId::UiDemoButtonIdle,
        MessageId::UiDemoButtonHovered,
        MessageId::UiDemoButtonPressed,
        MessageId::UiDemoButtonUnnamed,
        MessageId::UiDemoButtonsWidths,
        MessageId::UiDemoButtonFit,
        MessageId::UiDemoButtonFixed,
        MessageId::UiDemoButtonGrow,
        MessageId::UiDemoLayoutNest,
        MessageId::UiDemoAlignStart,
        MessageId::UiDemoAlignCenter,
        MessageId::UiDemoAlignEnd,
        MessageId::UiDemoAcrossAxis,
        MessageId::UiDemoPanelIsColumn,
        MessageId::UiDemoPanelInset,
        MessageId::UiDemoFieldOwned,
        MessageId::UiDemoFieldPlaceholder,
        MessageId::UiDemoFieldKeys,
        MessageId::UiDemoFieldHolding,
        MessageId::UiDemoListOpenBit,
        MessageId::UiDemoNoneChosen,
        MessageId::UiDemoListOverlay,
        MessageId::UiDemoAccentAmber,
        MessageId::UiDemoAccentMint,
        MessageId::UiDemoAccentRose,
        MessageId::UiDemoFocusKeys,
        MessageId::UiDemoButtonFirst,
        MessageId::UiDemoButtonSecond,
        MessageId::UiDemoButtonThird,
        MessageId::UiDemoFocusRingFills,
        MessageId::UiDemoFocusedId,
        MessageId::UiDemoThemeColours,
        MessageId::UiDemoRectsSays,
        MessageId::UiDemoRowIsNamed,
        MessageId::UiDemoBarFromRect,
        MessageId::UiDemoUndeclaredId,
        MessageId::UiDemoShrinkProportion,
        MessageId::UiDemoTooWide,
        MessageId::UiDemoAlsoTooWide,
        MessageId::UiDemoNoClipping,
        MessageId::UiDemoLayoutsJob,
        MessageId::UiDemoShowing,
        MessageId::UiDemoAccentChosen,
        MessageId::UiDemoSubmitted,
        MessageId::UiDemoCancelled,
        MessageId::UiDemoPressedWidget,
    };

    /**
     * @brief The id's own name, for diagnostics and for the text a total
     *        lookup falls back on.
     * @param id The id to name.
     * @return The enumerator's name, or `"?"` for a value that is not one
     *         of the enumerators.
     */
    [[nodiscard]] std::string_view nameOf(MessageId id) noexcept;

} // namespace antwika::i18n
