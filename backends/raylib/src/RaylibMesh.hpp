#pragma once

#include <cstddef>

#include <raylib.h>

#include <antwika/gfx/IMesh.hpp>

namespace antwika::gfx::raylib
{

    class RaylibRenderer;

    /**
     * @brief Geometry uploaded to raylib's one GL context.
     *
     * RaylibTexture's opposite number for triangles, and it plays the
     * same game with the same rules: it registers with the renderer that
     * made it, so that renderer can unload it while the context is still
     * alive.
     * raylib frees a mesh through the context CloseWindow() destroys, and
     * a mesh is free to outlive its window, so one of the two has to know
     * about the other.
     *
     * The ::Mesh held here owns malloc'd copies of the vertex arrays as
     * well as the GPU buffers, because UnloadMesh() frees both.
     */
    class RaylibMesh final : public IMesh
    {
    public:
        /**
         * @brief Construct the mesh and register it with its owner.
         * @param owner The renderer that created it, and the only one
         * that may draw it; must outlive this object or call detach().
         * @param mesh The uploaded raylib mesh, owned by this object.
         */
        RaylibMesh(RaylibRenderer &owner, ::Mesh mesh);

        RaylibMesh(const RaylibMesh &) = delete;
        RaylibMesh(RaylibMesh &&) = delete;

        RaylibMesh &operator=(const RaylibMesh &) = delete;
        RaylibMesh &operator=(RaylibMesh &&) = delete;

        /**
         * @brief Unload the mesh, if the renderer has not already.
         */
        ~RaylibMesh() override;

        /**
         * @brief Get how many vertices this mesh holds.
         * @return Exactly the number the MeshData it came from held.
         */
        [[nodiscard]] std::size_t vertexCount() const override;

        /**
         * @brief Get how many triangles this mesh holds.
         * @return Exactly the number that data described.
         */
        [[nodiscard]] std::size_t triangleCount() const override;

        /**
         * @brief Check which renderer this mesh belongs to.
         * @param candidate The renderer proposing to draw it.
         * @return True when candidate is the renderer that created it.
         */
        [[nodiscard]] bool belongsTo(
            const RaylibRenderer &candidate) const noexcept;

        /**
         * @brief Get the raylib mesh to draw.
         * @return The mesh; only meaningful while loaded.
         */
        [[nodiscard]] const ::Mesh &raw() const noexcept;

        /**
         * @brief Check whether this mesh is still on the GPU.
         * @return False once its renderer has unloaded it.
         */
        [[nodiscard]] bool isLoaded() const noexcept;

        /**
         * @brief Give up the mesh, which the renderer has unloaded.
         *
         * Leaves this object valid but drawing nothing, so a caller
         * holding it past its renderer's life is safe rather than sorry.
         * The counts survive, since they are this object's own copies
         * rather than anything raylib still owns.
         */
        void forgetRenderer() noexcept;

    private:
        RaylibRenderer *owner;
        ::Mesh mesh;
        std::size_t vertices;
        std::size_t triangles;
        bool loaded = true;
    };

} // namespace antwika::gfx::raylib
