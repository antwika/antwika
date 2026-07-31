#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace antwika::gfx
{

    /**
     * @brief The 3D vector these types are all built out of.
     *
     * **Render-side only.** Every type in this header, and everything
     * built on one -- Transform, Camera3D, Vertex3D, MeshData -- is
     * floating point, and floating point may never appear in anything a
     * replay reproduces.
     * A tick's state must land on the same bits under every compiler,
     * optimisation level and instruction set, and these do not promise
     * that.
     *
     * That is not a restriction rendering feels, because rendering is
     * already a write-only projection of state (see IRenderer): a
     * matrix computed here reaches a screen and nothing else.
     * Simulation state that happens to be spatial stays integral --
     * apps/game's camera is deliberately simulation state, holds whole
     * tile sizes rather than a scale factor, and is not one of these.
     *
     * These are GLM's own types rather than wrappers, so a caller can
     * use the whole of GLM on them without conversion.
     */
    using Vec2 = glm::vec2;

    /**
     * @brief A position, direction or scale in three dimensions.
     *
     * Render-side only, for the reason Vec2 gives.
     */
    using Vec3 = glm::vec3;

    /**
     * @brief A homogeneous position or colour in four dimensions.
     *
     * Render-side only, for the reason Vec2 gives.
     */
    using Vec4 = glm::vec4;

    /**
     * @brief A column-major 4x4 transformation matrix.
     *
     * Column-major, and applied to a column vector on its right, which
     * is GLM's convention and OpenGL's: a model-view-projection is
     * projection * view * model, read right to left.
     *
     * Render-side only, for the reason Vec2 gives.
     */
    using Mat4 = glm::mat4;

    /**
     * @brief The identity matrix.
     * @return A matrix that leaves every vector it is applied to
     * unchanged.
     */
    [[nodiscard]] inline Mat4 identityMatrix()
    {
        return Mat4(1.0F);
    }

} // namespace antwika::gfx
