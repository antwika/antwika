#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/voxel/VoxelPosition.hpp>

namespace antwika::component
{

    enum class PadKind : std::uint8_t
    {
        Start,
        Exit,
        Checkpoint,
    };

    [[nodiscard]] constexpr PadKind getLastEnumerator(PadKind) noexcept
    {
        return PadKind::Checkpoint;
    }

    inline constexpr std::size_t kPadKindCount = enums::kCount<PadKind>;

    inline constexpr std::array<PadKind, kPadKindCount> kEveryPadKind =
        enums::kAll<PadKind>;

    struct Pad final
    {
        voxel::VoxelPosition position{};

        std::uint8_t kind = 0;

        [[nodiscard]] bool operator==(const Pad &other) const = default;
    };

}
