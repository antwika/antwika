#pragma once

#include <array>
#include <cstdint>
#include <compare>

namespace antwika::voxel
{

    inline constexpr float kVoxelSide = 1.0F;

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

    enum class StairHalf : std::uint8_t
    {
        Any,
        Lower,
        Upper,
    };

    inline constexpr std::array<StairHalf, 3> kEveryStairHalf{
        StairHalf::Any, StairHalf::Lower, StairHalf::Upper};

    enum class StairPart : std::uint8_t
    {
        Any,
        Front,
        Side,
    };

    inline constexpr std::array<StairPart, 3> kEveryStairPart{
        StairPart::Any, StairPart::Front, StairPart::Side};

    inline constexpr std::array<Kind, 4> kEveryKind{
        Kind::Normal, Kind::Water, Kind::Ramp, Kind::Ladder};

    struct VoxelCell final
    {
        std::int32_t x = 0;
        std::int32_t y = 0;
        std::int32_t z = 0;

        Kind kind = Kind::Normal;

        Facing facing = Facing::Any;

        [[nodiscard]] bool operator==(const VoxelCell &other) const
        {
            return x == other.x && y == other.y && z == other.z;
        }

        [[nodiscard]] std::strong_ordering operator<=>(
            const VoxelCell &other) const
        {
            if (const auto xOrder = x <=> other.x; xOrder != 0)
            {
                return xOrder;
            }

            if (const auto yOrder = y <=> other.y; yOrder != 0)
            {
                return yOrder;
            }

            return z <=> other.z;
        }
    };

    [[nodiscard]] bool occludes(Kind neighbourKind, Kind selfKind);

}
