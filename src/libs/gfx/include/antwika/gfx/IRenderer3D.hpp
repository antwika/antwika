#pragma once

#include <memory>

#include "antwika/gfx/Camera3D.hpp"
#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/IMesh.hpp"
#include "antwika/gfx/Math3D.hpp"
#include "antwika/gfx/MeshData.hpp"

namespace antwika::gfx
{

    /**
     * @brief Draws triangles into one window's drawable area.
     *
     * A sibling of IRenderer rather than more methods on it, and that
     * is the load-bearing decision here.
     * IRenderer is implemented by every backend and by test doubles all
     * over the repository; a new pure virtual on it would break every
     * one of them at once, and a backend with no 3D path would be made
     * to write no-ops for calls it can never honour.
     * Splitting the interface lets a backend say "no 3D" by not
     * offering one -- IRenderer::renderer3d() returns null -- which is
     * a question a caller can ask, unlike a call that silently does
     * nothing.
     *
     * Everything IRenderer refuses, this refuses too: no pixel
     * read-back, no render target, no screenshot, no way to reach the
     * framework object behind a mesh.
     * Rendering stays a write-only projection of state, so a replay
     * reproduces the same ticks whether anything was drawn or not.
     *
     * A 3D renderer draws into the same drawable area as the 2D one it
     * came from, and clearing and presenting stay on IRenderer: there
     * is one frame, drawn by both.
     */
    class IRenderer3D
    {
    public:
        virtual ~IRenderer3D() = default;

        /**
         * @brief Create a mesh this renderer can draw.
         *
         * A mesh belongs to the renderer that made it.
         * The returned mesh owns itself and may outlive this renderer:
         * destroying it afterwards is safe, and drawing it afterwards
         * draws nothing.
         *
         * The data is uploaded rather than kept, so it may be destroyed
         * as soon as this returns.
         *
         * Creation reports failure by throwing, unlike the drawing call
         * below, for the reason IRenderer::createTexture() does: a
         * caller that cannot have the resource it asked for has nothing
         * to carry on with.
         * @param mesh The geometry to upload.
         * @return The new mesh, never null.
         * @throws GfxError If the data is not complete, or if the
         * renderer could not hold the geometry.
         */
        [[nodiscard]] virtual std::unique_ptr<IMesh> createMesh(
            const MeshData &mesh) = 0;

        /**
         * @brief Draw a mesh through a camera.
         *
         * Never throws, like every other drawing call.
         * Nothing is drawn when the mesh came from another renderer or
         * when its window has closed.
         * @param mesh The geometry to draw.
         * @param model Takes the mesh's own space to world space;
         * Transform::matrix() builds one.
         * @param camera Takes world space to clip space.
         * @param tint Multiplied into every vertex colour, so an opaque
         * white tint draws the mesh's own colours unchanged.
         */
        virtual void drawMesh(
            const IMesh &mesh,
            const Mat4 &model,
            const Camera3D &camera,
            Color tint) = 0;
    };

} // namespace antwika::gfx
