#pragma once

#include <array>
#include <compare>
#include <cstdint>

namespace antwika::voxel
{

    enum class Kind : std::uint8_t
    {
        Normal,
        Water,
        Ramp,
        Ladder,
    };

    enum class Facing : std::uint8_t
    {
        Any,
        East,
        West,
        North,
        South,
    };

    inline constexpr std::array<Facing, 5> kEveryFacing{
        Facing::Any,
        Facing::East,
        Facing::West,
        Facing::North,
        Facing::South};

    inline constexpr std::array<Kind, 4> kEveryKind{
        Kind::Normal, Kind::Water, Kind::Ramp, Kind::Ladder};

    struct VoxelMaterial final
    {
        Kind kind = Kind::Normal;

        Facing facing = Facing::Any;

        [[nodiscard]] bool operator==(const VoxelMaterial &other) const
            = default;

        [[nodiscard]] auto operator<=>(const VoxelMaterial &other) const
            = default;
    };

    [[nodiscard]] bool occludes(Kind neighbourKind, Kind selfKind);

}
