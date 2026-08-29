#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <antwika/gfx/MeshData.hpp>

namespace antwika::render
{

    inline constexpr std::array<std::array<std::size_t, 3>, 2>
        kWindingsThatDodgeBackFaceCulling{{{0, 1, 2}, {0, 2, 1}}};

    inline void layDoubleSidedTriangle(
        gfx::MeshData &mesh,
        const std::array<std::uint32_t, 3> &corners)
    {
        for (const auto &winding : kWindingsThatDodgeBackFaceCulling)
        {
            for (const auto place : winding)
            {
                mesh.indices.push_back(corners.at(place));
            }
        }
    }

}
