#pragma once

#include <array>
#include <cstdint>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/voxel/VoxelMaterial.hpp>

#include "antwika/editor/editor/state/PanelSizes.hpp"

namespace antwika::editor
{

    enum class Tool : std::uint8_t
    {
        Select,
        Brush,
        Picker,
        Lamp,
        Start,
        Exit,
        Stamp,
        Character,
        Checkpoint,

        Food,

        Water,

        Eraser,
    };

    [[nodiscard]] constexpr Tool getLastEnumerator(Tool) noexcept
    {
        return Tool::Eraser;
    }

    enum class Paint : std::uint8_t
    {
        Brush,
        Line,
        Fill,
        Select,
        Rect,
        Circle,
    };

    [[nodiscard]] constexpr Paint getLastEnumerator(Paint) noexcept
    {
        return Paint::Circle;
    }

    enum class View : std::uint8_t
    {
        World,
        Atlases,
        Character,
        Icons,
        Plan,
        Gizmos,
    };

    [[nodiscard]] constexpr View getLastEnumerator(View) noexcept
    {
        return View::Gizmos;
    }

    inline constexpr std::array<View, enums::kCount<View>> kEveryView =
        enums::kAll<View>;

    [[nodiscard]] constexpr View getViewAfter(const View view) noexcept
    {
        return enums::wrapToEnum<View>(enums::index(view) + 1);
    }

    [[nodiscard]] constexpr View getViewBefore(const View view) noexcept
    {
        return enums::wrapToEnum<View>(
            enums::index(view) + enums::kCount<View> - 1);
    }

    struct Preferences final
    {
        Tool tool = Tool::Brush;

        Paint paint = Paint::Brush;

        View view = View::World;

        voxel::Kind kind = voxel::Kind::Normal;

        bool lighting = true;

        bool showRuleLines = true;

        bool grid = true;

        bool showPlacementGhost = true;

        bool lampSight = true;

        bool cameraFollows = true;

        bool hideAboveLevel = false;

        PanelSizes panelSizes;

        [[nodiscard]] bool operator==(const Preferences &other) const
            = default;
    };

}
