#pragma once

#include <array>
#include <cstdint>

#include <antwika/voxel/VoxelCell.hpp>

namespace antwika::map
{

    enum class Tool : std::uint8_t
    {
        Brush,
        Picker,
        Lamp,
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

    enum class Paint : std::uint8_t
    {
        Brush,
        Line,
        Fill,
        Select,
        Rect,
        Circle,
    };

    enum class View : std::uint8_t
    {
        World,
        Atlases,
        Character,
        Icons,
        Plan,
    };

    [[nodiscard]] constexpr View lastEnumerator(View) noexcept
    {
        return View::Plan;
    }

    inline constexpr std::array<View, 5> kEveryView{
        View::World,
        View::Atlases,
        View::Character,
        View::Icons,
        View::Plan};

    struct Settings final
    {
        bool lighting = true;

        bool showRuleLines = true;

        Tool tool = Tool::Brush;

        Paint paint = Paint::Brush;

        View view = View::World;

        voxel::Kind kind = voxel::Kind::Normal;

        bool grid = true;

        bool showPlacementGhost = true;

        bool lampSight = true;

        bool cameraFollows = true;

        bool hideAboveLevel = false;

        bool cornersJoined = false;

        [[nodiscard]] bool operator==(const Settings &other) const
            = default;
    };

}
