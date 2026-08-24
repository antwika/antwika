#pragma once

#include <array>
#include <compare>
#include <cstdint>

#include <antwika/enums/Enumeration.hpp>

namespace antwika::voxel
{

    enum class Kind : std::uint8_t
    {
        Normal,
        Water,
        Ramp,
    };

    [[nodiscard]] constexpr Kind getLastEnumerator(Kind) noexcept
    {
        return Kind::Ramp;
    }

    enum class Facing : std::uint8_t
    {
        Any,
        East,
        West,
        North,
        South,
    };

    [[nodiscard]] constexpr Facing getLastEnumerator(Facing) noexcept
    {
        return Facing::South;
    }

    inline constexpr std::array<Facing, enums::kCount<Facing>>
        kEveryFacing = enums::kAll<Facing>;

    inline constexpr std::array<Kind, enums::kCount<Kind>> kEveryKind =
        enums::kAll<Kind>;

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
