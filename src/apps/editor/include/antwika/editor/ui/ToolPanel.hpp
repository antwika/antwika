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

#include <antwika/voxel/VoxelStairs.hpp>

#include "antwika/editor/Preferences.hpp"

namespace antwika::editor
{

    enum class ToolButton : std::uint8_t
    {
        StoneCube,
        WaterCube,
        RampCube,
        Picker,
        Stamp,
        Rubber,
        Select,
        Lamp,
        Start,
        Exit,
        Character,
        Checkpoint,
        Food,
        Water,
    };

    [[nodiscard]] constexpr ToolButton getLastEnumerator(ToolButton) noexcept
    {
        return ToolButton::Water;
    }

    inline constexpr std::array<ToolButton, enums::kCount<ToolButton>>
        kEveryToolButton = enums::kAll<ToolButton>;

    inline constexpr std::array<Paint, enums::kCount<Paint>>
        kEveryPaint = enums::kAll<Paint>;

    [[nodiscard]] gfx::Rect getMirrorIcon();

    inline constexpr float kToolButtonSide = 16.0F;

    inline constexpr float kIconSide = 16.0F;

    [[nodiscard]] gfx::Rect iconOf(ToolButton whichButton);

    [[nodiscard]] gfx::Rect iconOf(Paint whichPaint);

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

    [[nodiscard]] widget::WidgetId getToolWidget(ToolButton whichButton);

    [[nodiscard]] widget::WidgetId getPaintWidget(Paint whichPaint);

    [[nodiscard]] widget::WidgetId getKindWidget(voxel::Kind whichKind);

    [[nodiscard]] widget::WidgetId getFacingWidget(voxel::Facing whichFacing);

    [[nodiscard]] widget::WidgetId getLevelWidget(voxel::StairHalf whichHalf);

    [[nodiscard]] widget::WidgetId getPartWidget(voxel::StairPart whichPart);

    [[nodiscard]] widget::WidgetId getCharacterWidget(std::size_t characterIndex);

    inline constexpr float kToolButtonGap = 4.0F;

    inline constexpr float kToolPanelPad = 4.0F;

    inline constexpr float kToolPanelTop = 24.0F;

    inline constexpr float kToolPanelLeft = 4.0F;

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

    [[nodiscard]] std::optional<Paint> paintFor(
        input::Key key, bool ctrlHeld);

}
