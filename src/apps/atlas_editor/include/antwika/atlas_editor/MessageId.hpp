#pragma once

#include <cstdint>

namespace antwika::atlas_editor
{

    enum class MessageId : std::uint16_t
    {
        ToolPaint,

        ToolErase,

        ToolFill,

        ToolPick,

        ToolSelect,

        ToolLine,

        ToolEllipse,

        MenuFile,

        MenuView,

        New,

        Quit,

        ZoomIn,

        ZoomOut,

        ResetView,

        Grid,

        Guides,

        PixelGrid,

        Load,

        Save,

        Close,

        FileName,

        PixelUnknown,

        PixelAt,

        Slot,

        SelectionSize,

        Unsaved,

        Saved,

        SaveFailed,

        NothingToLoad,

        Loaded,

        LoadFailed,

        NewAtlas,

        AtlasKind,

        KindIsometric,

        KindFlat,

        SpriteWidth,

        SpriteHeight,

        Columns,

        Rows,

        PivotX,

        PivotY,

        IsometricWidth,

        IsometricHeight,

        Create,

        MetaSlots,

        MetaSprite,

        MetaPivot,

        MetaIsometric,

        MetaKind,

        AtlasTooSmall,

        Pivot,

        PointerBorder,

        Preview,

        PreviewFocus,

        PresetOneByOne,

        PresetTwoByTwo,

        PresetThreeByThree,

        PresetFourByFour,

        Presets,

        Count,
    };

}
