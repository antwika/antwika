#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/Math3D.hpp"
#include "antwika/gfx/Vertex3D.hpp"

namespace antwika::gfx
{

    struct MeshData final
    {
        std::vector<Vertex3D> vertices;

        std::vector<std::uint32_t> indices;

        [[nodiscard]] bool isComplete() const;

        [[nodiscard]] std::size_t getTriangleCount() const;

        [[nodiscard]] bool operator==(const MeshData &other) const
            = default;
    };

    [[nodiscard]] std::vector<MeshData> getSplitMesh(
        const MeshData &mesh, std::size_t maxVertices);

}
