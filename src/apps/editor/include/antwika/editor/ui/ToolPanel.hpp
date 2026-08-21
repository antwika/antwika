#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/ui/WidgetId.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/voxelmap/Voxel.hpp>

#include <antwika/map/Settings.hpp>

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

    inline constexpr std::array kEveryToolButton{
        ToolButton::Brush,
        ToolButton::Picker,
        ToolButton::FreeLook,
        ToolButton::Lighting,
        ToolButton::Lamp,
        ToolButton::RuleLines,
        ToolButton::Start,
        ToolButton::Exit,
        ToolButton::Stamp,
        ToolButton::Figure,
        ToolButton::PressurePlate,
        ToolButton::Key,
        ToolButton::Door,
        ToolButton::Checkpoint,
        ToolButton::Food,
        ToolButton::Water,
        ToolButton::Eraser};

    inline constexpr std::array kEveryPaint{
        map::Paint::Brush,
        map::Paint::Line,
        map::Paint::Fill,
        map::Paint::Select,
        map::Paint::Rect,
        map::Paint::Circle};

    [[nodiscard]] gfx::Rect mirrorIcon();

    inline constexpr ui::WidgetId kMirrorWidget{203};

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

    [[nodiscard]] ui::WidgetId toolWidget(ToolButton whichButton);

    [[nodiscard]] ui::WidgetId paintWidget(map::Paint whichPaint);

    [[nodiscard]] ui::WidgetId kindWidget(voxel::Kind whichKind);

    [[nodiscard]] ui::WidgetId facingWidget(voxel::Facing whichFacing);

    [[nodiscard]] ui::WidgetId levelWidget(voxel::StairHalf whichHalf);

    inline constexpr ui::WidgetId kPartFrontWidget{180};

    inline constexpr ui::WidgetId kPartSideWidget{181};

    [[nodiscard]] ui::WidgetId partWidget(voxel::StairPart whichPart);

    inline constexpr ui::WidgetId kToolPanelWidget{64};

    inline constexpr ui::WidgetId kStatusBarWidget{192};

    inline constexpr ui::WidgetId kLayersPanelWidget{193};

    inline constexpr ui::WidgetId kPaletteWidget{194};

    inline constexpr ui::WidgetId kRailWidget{195};

    inline constexpr ui::WidgetId kPreviewWidget{196};

    inline constexpr ui::WidgetId kAddInkWidget{197};

    inline constexpr ui::WidgetId kInkOkWidget{198};

    inline constexpr ui::WidgetId kInkCancelWidget{199};

    inline constexpr ui::WidgetId kInkDeleteWidget{200};

    inline constexpr ui::WidgetId kQuitConfirmWidget{236};

    inline constexpr ui::WidgetId kQuitCancelWidget{237};

    inline constexpr ui::WidgetId kQuitAndSaveWidget{238};

    inline constexpr ui::WidgetId kGlowWidget{245};

    inline constexpr ui::WidgetId kAmbientWidget{246};

    inline constexpr ui::WidgetId kExitTargetWidget{247};

    inline constexpr ui::WidgetId kExitLockedWidget{402};

    inline constexpr ui::WidgetId kSheetPanelWidget{403};

    inline constexpr ui::WidgetId kDrawPanelWidget{404};

    inline constexpr ui::WidgetId kFigureNameWidget{248};

    inline constexpr ui::WidgetId kFigureLampWidget{254};

    inline constexpr ui::WidgetId kAddFigureWidget{249};

    inline constexpr ui::WidgetId kRemoveFigureWidget{250};

    inline constexpr ui::WidgetId kFigureLineWidget{251};

    inline constexpr ui::WidgetId kFigureLineAddWidget{252};

    [[nodiscard]] ui::WidgetId figureWidget(std::size_t figureIndex);

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
