#pragma once

#include <cstdint>

namespace antwika::ui_demo
{

    enum class MessageId : std::uint16_t
    {
        Title,

        PickPage,

        PageLabels,

        PageButtons,

        PageLayout,

        PageTextField,

        PageDropdown,

        PageFocus,

        PageTheme,

        PageRects,

        PageShrink,

        PageTextArea,

        LabelsLine,

        LabelsMuted,

        LabelsOwnInk,

        SpacerLeft,

        SpacerRight,

        ButtonsPress,

        ButtonCount,

        ButtonReset,

        PressedCount,

        ButtonsForced,

        ButtonIdle,

        ButtonHovered,

        ButtonPressed,

        ButtonUnnamed,

        ButtonsWidths,

        ButtonFit,

        ButtonFixed,

        ButtonGrow,

        LayoutNest,

        AlignStart,

        AlignCenter,

        AlignEnd,

        AcrossAxis,

        PanelIsColumn,

        PanelInset,

        FieldOwned,

        FieldPlaceholder,

        FieldKeys,

        FieldHolding,

        AreaOwned,

        AreaShowing,

        AreaPlaceholder,

        ListOpenBit,

        NoneChosen,

        ListOverlay,

        AccentAmber,

        AccentMint,

        AccentRose,

        FocusKeys,

        ButtonFirst,

        ButtonSecond,

        ButtonThird,

        FocusRingFills,

        FocusedId,

        ThemeColours,

        RectsSays,

        RowIsNamed,

        BarFromRect,

        UndeclaredId,

        ShrinkProportion,

        TooWide,

        AlsoTooWide,

        NoClipping,

        LayoutsJob,

        Showing,

        AccentChosen,

        Submitted,

        Cancelled,

        PressedWidget,

        Count,
    };

}
