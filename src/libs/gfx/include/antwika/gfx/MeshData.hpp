#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/Math3D.hpp"

namespace antwika::gfx
{

    struct Vertex3D final
    {
        Vec3 position{0.0F, 0.0F, 0.0F};
        Vec3 normal{0.0F, 0.0F, 1.0F};
        Vec2 texCoord{0.0F, 0.0F};
        Color color{255, 255, 255, 255};

        [[nodiscard]] bool operator==(const Vertex3D &other) const = default;
    };

    struct MeshData final
    {
        std::vector<Vertex3D> vertices;

        std::vector<std::uint32_t> indices;

        [[nodiscard]] bool isComplete() const;

        [[nodiscard]] std::size_t triangleCount() const;
    };

}
