#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/Math3D.hpp"

namespace antwika::gfx
{

    /**
     * @brief One corner of one triangle, ready to be uploaded.
     *
     * Render-side only, like everything in Math3D.hpp.
     * The colour is the 8-bit Color the 2D calls already use rather
     * than a float vector, so one palette serves the whole library and
     * a mesh's colours can be compared exactly in a test.
     */
    struct Vertex3D
    {
        Vec3 position{0.0F, 0.0F, 0.0F};
        Vec3 normal{0.0F, 0.0F, 1.0F};
        Vec2 texCoord{0.0F, 0.0F};
        Color color{255, 255, 255, 255};

        /**
         * @brief Compare two vertices field by field.
         * @param other The vertex to compare against.
         * @return True when every field matches exactly.
         */
        [[nodiscard]] bool operator==(const Vertex3D &other) const = default;
    };

    /**
     * @brief An indexed triangle list, in the caller's own memory.
     *
     * The counterpart to Bitmap: a plain value a caller fills in and
     * hands to IRenderer3D::createMesh(), which uploads it and keeps
     * nothing.
     * Triangles only -- no strips, no fans, no lines -- because one
     * primitive is one thing every backend agrees about, exactly as
     * there is one font.
     */
    struct MeshData
    {
        std::vector<Vertex3D> vertices;

        /// Three indices per triangle, each addressing `vertices`.
        std::vector<std::uint32_t> indices;

        /**
         * @brief Check the indices describe whole triangles that exist.
         * @return True when there is at least one triangle, the index
         * count is a multiple of three, and every index addresses a
         * vertex that is present.
         */
        [[nodiscard]] bool isComplete() const;

        /**
         * @brief Count the triangles this data describes.
         * @return The index count divided by three, rounded down.
         */
        [[nodiscard]] std::size_t triangleCount() const;
    };

} // namespace antwika::gfx
