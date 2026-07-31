#pragma once

#include <cstddef>

#include "antwika/gfx/IMesh.hpp"

namespace antwika::gfx::detail
{

    /**
     * @brief Mesh that remembers its counts and holds no geometry.
     *
     * The headless counterpart to a real backend's mesh, and NullTexture's
     * opposite number for triangles: it is created, reported on and
     * destroyed exactly like one, so the same application code runs with
     * no display and no framework.
     */
    class NullMesh final : public IMesh
    {
    public:
        /**
         * @brief Construct the mesh.
         * @param vertices How many vertices the data it stands in for
         * held.
         * @param triangles How many triangles that data described.
         */
        NullMesh(std::size_t vertices, std::size_t triangles);

        NullMesh(const NullMesh &) = delete;
        NullMesh(NullMesh &&) = delete;

        NullMesh &operator=(const NullMesh &) = delete;
        NullMesh &operator=(NullMesh &&) = delete;

        /**
         * @brief Get the vertex count this mesh was created with.
         * @return That count, unchanged.
         */
        [[nodiscard]] std::size_t vertexCount() const override;

        /**
         * @brief Get the triangle count this mesh was created with.
         * @return That count, unchanged.
         */
        [[nodiscard]] std::size_t triangleCount() const override;

    private:
        std::size_t vertices;
        std::size_t triangles;
    };

} // namespace antwika::gfx::detail
