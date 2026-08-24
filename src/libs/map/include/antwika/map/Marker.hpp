#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <optional>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/map/Settings.hpp>
#include <antwika/voxel/VoxelPosition.hpp>

namespace antwika::map
{

    enum class Marker : std::uint8_t
    {
        Key,
        Door,
        Checkpoint,
        Food,
        Water,
    };

    [[nodiscard]] constexpr Marker getLastEnumerator(Marker) noexcept
    {
        return Marker::Water;
    }

    inline constexpr std::array<Marker, enums::kCount<Marker>> kEveryMarker =
        enums::kAll<Marker>;

    struct Markers final
    {
        std::array<std::vector<voxel::VoxelPosition>, enums::kCount<Marker>>
            positionsByKind{};

        [[nodiscard]] std::vector<voxel::VoxelPosition> &positionsOf(
            const Marker marker)
        {
            return positionsByKind.at(enums::index(marker));
        }

        [[nodiscard]] const std::vector<voxel::VoxelPosition> &positionsOf(
            const Marker marker) const
        {
            return positionsByKind.at(enums::index(marker));
        }

        [[nodiscard]] bool operator==(const Markers &other) const = default;
    };

    [[nodiscard]] std::optional<Marker> markerFor(Tool tool);

}
