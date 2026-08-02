#pragma once

#include <cstdint>

namespace antwika::ui_demo
{

    /**
     * @brief Every string the ui showcase shows, as symbolic ids.
     *
     * They live here rather than in antwika::i18n because a library
     * that enumerated its consumers' strings would be a library naming
     * its consumers.
     * What keeps that safe is that the list of every id there is, both
     * catalogues and the completeness check over them are in this
     * module too: see the MessageSet concept in
     * <antwika/i18n/MessageSet.hpp> and the suite MessagesTest.cpp
     * instantiates.
     *
     * A MessageId is never persisted, so its numbering is free and
     * adding, reordering or removing one needs no migration.
     */
    enum class MessageId : std::uint16_t
    {
        /**
         * @brief The heading over every page.
         */
        Title,

        /**
         * @brief The page picker with nothing chosen yet.
         */
        PickPage,

        /**
         * @brief The page of labels.
         */
        PageLabels,

        /**
         * @brief The page of buttons.
         */
        PageButtons,

        /**
         * @brief The page of nested containers.
         */
        PageLayout,

        /**
         * @brief The page holding a text field.
         */
        PageTextField,

        /**
         * @brief The page holding a second list.
         */
        PageDropdown,

        /**
         * @brief The page walked with the keyboard.
         */
        PageFocus,

        /**
         * @brief The page of theme colours.
         */
        PageTheme,

        /**
         * @brief The page reading a layout back.
         */
        PageRects,

        /**
         * @brief The page with less room than its children want.
         */
        PageShrink,

        /**
         * @brief The text-area page's name.
         */
        PageTextArea,

        /**
         * @brief What a plain label is.
         */
        LabelsLine,

        /**
         * @brief What a muted label is for.
         */
        LabelsMuted,

        /**
         * @brief A label takes a colour.
         */
        LabelsOwnInk,

        /**
         * @brief The label left of a growing spacer.
         */
        SpacerLeft,

        /**
         * @brief The label right of a growing spacer.
         */
        SpacerRight,

        /**
         * @brief When a button activates.
         */
        ButtonsPress,

        /**
         * @brief The button counting a press.
         */
        ButtonCount,

        /**
         * @brief The button clearing that count.
         */
        ButtonReset,

        /**
         * @brief How many presses have been counted, `{0}`.
         */
        PressedCount,

        /**
         * @brief An appearance the caller decided.
         */
        ButtonsForced,

        /**
         * @brief A button forced to look idle.
         */
        ButtonIdle,

        /**
         * @brief A button forced to look hovered.
         */
        ButtonHovered,

        /**
         * @brief A button forced to look pressed.
         */
        ButtonPressed,

        /**
         * @brief A button with no id, which nothing can hit.
         */
        ButtonUnnamed,

        /**
         * @brief The three widths a button takes.
         */
        ButtonsWidths,

        /**
         * @brief A button as wide as its own label.
         */
        ButtonFit,

        /**
         * @brief A button of a stated width.
         */
        ButtonFixed,

        /**
         * @brief A button taking what room is left.
         */
        ButtonGrow,

        /**
         * @brief How deep a layout may nest.
         */
        LayoutNest,

        /**
         * @brief Aligned to the start of the axis.
         */
        AlignStart,

        /**
         * @brief Aligned to the middle of the axis.
         */
        AlignCenter,

        /**
         * @brief Aligned to the end of the axis.
         */
        AlignEnd,

        /**
         * @brief Which axis an alignment is across.
         */
        AcrossAxis,

        /**
         * @brief What a panel is.
         */
        PanelIsColumn,

        /**
         * @brief What else a panel is.
         */
        PanelInset,

        /**
         * @brief Who owns a field's characters.
         */
        FieldOwned,

        /**
         * @brief An empty field's prompt.
         */
        FieldPlaceholder,

        /**
         * @brief What the two keys do to a field.
         */
        FieldKeys,

        /**
         * @brief What the field holds right now, `{0}`.
         */
        FieldHolding,

        /**
         * @brief What a text area retains between frames: nothing.
         */
        AreaOwned,

        /**
         * @brief The line the pane is showing at its top, `{0}`.
         */
        AreaShowing,

        /**
         * @brief An empty area's prompt.
         */
        AreaPlaceholder,

        /**
         * @brief Who owns a list's open flag.
         */
        ListOpenBit,

        /**
         * @brief The accent list with nothing chosen.
         */
        NoneChosen,

        /**
         * @brief Where an open list is drawn.
         */
        ListOverlay,

        /**
         * @brief The first accent colour.
         */
        AccentAmber,

        /**
         * @brief The second accent colour.
         */
        AccentMint,

        /**
         * @brief The third accent colour.
         */
        AccentRose,

        /**
         * @brief The keys that walk a row of buttons.
         */
        FocusKeys,

        /**
         * @brief The first button of the focus row.
         */
        ButtonFirst,

        /**
         * @brief The second button of the focus row.
         */
        ButtonSecond,

        /**
         * @brief The third button of the focus row.
         */
        ButtonThird,

        /**
         * @brief What the focus ring is made of.
         */
        FocusRingFills,

        /**
         * @brief Which widget has focus, `{0}` being its id.
         */
        FocusedId,

        /**
         * @brief What a ui::Theme decides on a widget's behalf.
         */
        ThemeColours,

        /**
         * @brief What Frame::rects answers.
         */
        RectsSays,

        /**
         * @brief The row whose rectangle is read.
         */
        RowIsNamed,

        /**
         * @brief The bar drawn from that rectangle.
         */
        BarFromRect,

        /**
         * @brief An id this frame never declared.
         */
        UndeclaredId,

        /**
         * @brief What too little room does to children.
         */
        ShrinkProportion,

        /**
         * @brief The first of two oversized buttons.
         */
        TooWide,

        /**
         * @brief The second of two oversized ones.
         */
        AlsoTooWide,

        /**
         * @brief There is no clipping, first of two lines.
         */
        NoClipping,

        /**
         * @brief There is no clipping, second of two lines.
         */
        LayoutsJob,

        /**
         * @brief Which page was just chosen, `{0}`.
         */
        Showing,

        /**
         * @brief Which accent was just chosen, `{0}`.
         */
        AccentChosen,

        /**
         * @brief The field was submitted holding `{0}`.
         */
        Submitted,

        /**
         * @brief The field was given up on.
         */
        Cancelled,

        /**
         * @brief A widget with no other answer was pressed, `{0}` being its id.
         */
        PressedWidget,

        /**
         * @brief How many ids there are; not an id itself.
         *
         * Messages.cpp static_asserts its name table against this,
         * which is what makes an enumerator nobody listed a build
         * failure rather than a string that is silently in no
         * catalogue.
         */
        Count,
    };

} // namespace antwika::ui_demo
