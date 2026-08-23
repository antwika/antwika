#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/widget/WidgetId.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/voxelmap/Voxel.hpp>

#include <antwika/map/Settings.hpp>
#include <antwika/voxel/VoxelStairs.hpp>

namespace antwika::editor
{

    enum class ToolButton : std::uint8_t
    {
        Brush,
        Picker,
        FreeLook,
        Lighting,
        Lamp,
        RuleLines,
        Start,
        Exit,
        Stamp,
        Figure,
        PressurePlate,
        Key,
        Door,
        Checkpoint,
        Food,
        Water,
        Eraser,
    };

    [[nodiscard]] constexpr ToolButton lastEnumerator(ToolButton) noexcept
    {
        return ToolButton::Eraser;
    }

    inline constexpr std::array<ToolButton, enums::kCount<ToolButton>>
        kEveryToolButton = enums::kAll<ToolButton>;

    inline constexpr std::array<map::Paint, enums::kCount<map::Paint>>
        kEveryPaint = enums::kAll<map::Paint>;

    [[nodiscard]] gfx::Rect mirrorIcon();

    inline constexpr widget::WidgetId kMirrorWidget{203};

    inline constexpr float kToolButtonSide = 16.0F;

    inline constexpr float kIconSide = 16.0F;

    [[nodiscard]] gfx::Rect iconOf(ToolButton whichButton);

    [[nodiscard]] gfx::Rect iconOf(map::Paint whichPaint);

    inline constexpr std::array<antwika::voxel::Facing, 4> kMarkedFacings{
        voxel::Facing::West,
        voxel::Facing::East,
        voxel::Facing::North,
        voxel::Facing::South};

    inline constexpr std::array<antwika::voxel::StairHalf, 2>
        kMarkedStairHalves{
        voxel::StairHalf::Lower, voxel::StairHalf::Upper};

    inline constexpr std::array<antwika::voxel::StairPart, 2> kMarkedParts{
        voxel::StairPart::Front, voxel::StairPart::Side};

    [[nodiscard]] gfx::Rect iconOf(voxel::Kind whichKind);

    [[nodiscard]] gfx::Rect iconOf(voxel::Facing whichFacing);

    [[nodiscard]] gfx::Rect iconOf(voxel::StairHalf whichHalf);

    [[nodiscard]] widget::WidgetId toolWidget(ToolButton whichButton);

    [[nodiscard]] widget::WidgetId paintWidget(map::Paint whichPaint);

    [[nodiscard]] widget::WidgetId kindWidget(voxel::Kind whichKind);

    [[nodiscard]] widget::WidgetId facingWidget(voxel::Facing whichFacing);

    [[nodiscard]] widget::WidgetId levelWidget(voxel::StairHalf whichHalf);

    inline constexpr widget::WidgetId kPartFrontWidget{180};

    inline constexpr widget::WidgetId kPartSideWidget{181};

    [[nodiscard]] widget::WidgetId partWidget(voxel::StairPart whichPart);

    inline constexpr widget::WidgetId kToolPanelWidget{64};

    inline constexpr widget::WidgetId kStatusBarWidget{192};

    inline constexpr widget::WidgetId kLayersPanelWidget{193};

    inline constexpr widget::WidgetId kPaletteWidget{194};

    inline constexpr widget::WidgetId kRailWidget{195};

    inline constexpr widget::WidgetId kPreviewWidget{196};

    inline constexpr widget::WidgetId kAddInkWidget{197};

    inline constexpr widget::WidgetId kInkOkWidget{198};

    inline constexpr widget::WidgetId kInkCancelWidget{199};

    inline constexpr widget::WidgetId kInkDeleteWidget{200};

    inline constexpr widget::WidgetId kQuitConfirmWidget{236};

    inline constexpr widget::WidgetId kQuitCancelWidget{237};

    inline constexpr widget::WidgetId kQuitAndSaveWidget{238};

    inline constexpr widget::WidgetId kGlowWidget{245};

    inline constexpr widget::WidgetId kAmbientWidget{246};

    inline constexpr widget::WidgetId kExitTargetWidget{247};

    inline constexpr widget::WidgetId kExitLockedWidget{402};

    inline constexpr widget::WidgetId kSheetPanelWidget{403};

    inline constexpr widget::WidgetId kDrawPanelWidget{404};

    inline constexpr widget::WidgetId kFigureNameWidget{248};

    inline constexpr widget::WidgetId kFigureLampWidget{254};

    inline constexpr widget::WidgetId kAddFigureWidget{249};

    inline constexpr widget::WidgetId kRemoveFigureWidget{250};

    inline constexpr widget::WidgetId kFigureLineWidget{251};

    inline constexpr widget::WidgetId kFigureLineAddWidget{252};

    [[nodiscard]] widget::WidgetId figureWidget(std::size_t figureIndex);

    inline constexpr float kToolButtonGap = 4.0F;

    inline constexpr float kToolPanelPad = 4.0F;

    inline constexpr float kToolPanelTop = 24.0F;

    inline constexpr float kToolPanelLeft = 4.0F;

    [[nodiscard]] std::optional<ToolButton> toolFor(
        input::Key key, bool shiftHeld, bool ctrlHeld);

    struct Stroke final
    {
        gfx::PointF fromPoint{};

        gfx::PointF toPoint{};

        [[nodiscard]] bool operator==(const Stroke &other) const
            = default;
    };

    inline constexpr float kFacingSide =
        (kToolButtonSide - kToolButtonGap) / 2.0F;

    inline constexpr float kKindGap = 10.0F;

    [[nodiscard]] std::optional<antwika::voxel::Kind> kindFor(
        input::Key key, bool ctrlHeld);

    [[nodiscard]] std::optional<map::Paint> paintFor(
        input::Key key, bool ctrlHeld);

}
