#pragma once

#include <array>
#include <optional>

#include <antwika/enums/Enumeration.hpp>

#include "antwika/voxel/VoxelMaterial.hpp"

namespace antwika::voxel
{

    struct KindTraits final
    {
        Kind kind;

        bool solid;

        bool swimmable;

        bool climbable;

        bool ramped;
    };

    inline constexpr std::array<KindTraits, enums::kCount<Kind>>
        kKindTraits{{
            {.kind = Kind::Normal,
             .solid = true,
             .swimmable = false,
             .climbable = false,
             .ramped = false},
            {.kind = Kind::Water,
             .solid = false,
             .swimmable = true,
             .climbable = false,
             .ramped = false},
            {.kind = Kind::Ramp,
             .solid = true,
             .swimmable = false,
             .climbable = false,
             .ramped = true},
            {.kind = Kind::Ladder,
             .solid = false,
             .swimmable = false,
             .climbable = true,
             .ramped = false}}};

    static_assert(enums::tagsInOrder(kKindTraits, &KindTraits::kind));

    [[nodiscard]] constexpr bool isSolid(const Kind kind) noexcept
    {
        return enums::lookup(kKindTraits, kind).solid;
    }

    [[nodiscard]] constexpr bool isSwimmable(const Kind kind) noexcept
    {
        return enums::lookup(kKindTraits, kind).swimmable;
    }

    [[nodiscard]] constexpr bool isClimbable(const Kind kind) noexcept
    {
        return enums::lookup(kKindTraits, kind).climbable;
    }

    [[nodiscard]] constexpr bool isRamped(const Kind kind) noexcept
    {
        return enums::lookup(kKindTraits, kind).ramped;
    }

    [[nodiscard]] constexpr bool isSolid(
        const std::optional<Kind> kind) noexcept
    {
        return kind.has_value() && isSolid(*kind);
    }

    [[nodiscard]] constexpr bool isSwimmable(
        const std::optional<Kind> kind) noexcept
    {
        return kind.has_value() && isSwimmable(*kind);
    }

    [[nodiscard]] constexpr bool isClimbable(
        const std::optional<Kind> kind) noexcept
    {
        return kind.has_value() && isClimbable(*kind);
    }

    [[nodiscard]] constexpr bool isRamped(
        const std::optional<Kind> kind) noexcept
    {
        return kind.has_value() && isRamped(*kind);
    }

}
