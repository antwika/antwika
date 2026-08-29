#pragma once

#include <array>
#include <cstdint>
#include <string>

#include <antwika/rng/IRng.hpp>
#include <antwika/voxel/VoxelCell.hpp>
#include <antwika/wfc/CompatibilityTable.hpp>

#include "antwika/worldgen/ChunkShape.hpp"
#include "antwika/worldgen/Socket.hpp"

namespace antwika::worldgen
{

    inline constexpr std::size_t kCubeFaces = 6;

    struct Prototype final
    {
        std::string name{};

        voxel::Kind kind = voxel::Kind::Normal;

        voxel::Facing facing = voxel::Facing::Any;

        bool air = false;

        std::array<Socket, kCubeFaces> sockets{};

        std::uint8_t roles = 0;

        [[nodiscard]] bool operator==(const Prototype &other) const
            = default;
    };

}
