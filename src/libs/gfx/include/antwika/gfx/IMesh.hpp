#pragma once

#include <cstddef>

namespace antwika::gfx
{

    /**
     * @brief Geometry held by a renderer, ready to be drawn.
     *
     * Opaque for the reason ITexture is: there is no way to reach the
     * underlying framework object and no way to read a vertex back.
     * Read-back is the one thing that would let rendering feed the
     * simulation, which IRenderer's write-only projection exists to
     * prevent, and geometry is no different from pixels there.
     *
     * A mesh belongs to the renderer that created it.
     * Drawing it through any other renderer draws nothing, and
     * destroying it after that renderer's window has closed is safe.
     */
    class IMesh
    {
    public:
        virtual ~IMesh() = default;

        /**
         * @brief Count the vertices this mesh holds.
         * @return Exactly the number the MeshData it was created from
         * held.
         */
        [[nodiscard]] virtual std::size_t vertexCount() const = 0;

        /**
         * @brief Count the triangles this mesh holds.
         * @return Exactly MeshData::triangleCount() of the data it was
         * created from.
         */
        [[nodiscard]] virtual std::size_t triangleCount() const = 0;
    };

} // namespace antwika::gfx
