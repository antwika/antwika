#pragma once

#include <cstdint>

#include <antwika/gfx/Camera3D.hpp>
#include <antwika/gfx/IMesh.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/IRenderer3D.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/Size.hpp>

namespace antwika::gfx3d_demo
{

    using antwika::gfx::Camera3D;
    using antwika::gfx::IMesh;
    using antwika::gfx::IRenderer;
    using antwika::gfx::IRenderer3D;
    using antwika::gfx::Mat4;
    using antwika::gfx::Size;

    /**
     * @brief Draws the demo's picture: a cleared background, the cube
     * turned to where this tick puts it, and a caption over the top.
     *
     * Stateless and deterministic on purpose, exactly as gfx_demo's
     * scene is. The same tick and canvas always produce the same
     * matrices and the same drawing calls in the same order, which is
     * what makes the picture assertable against a mock renderer instead
     * of having to be looked at.
     *
     * **The turn is a function of the tick, never of a clock.** A demo
     * has no replay to reproduce, but the rule this project draws
     * everything by is that what a frame shows follows from the count of
     * ticks that reached it; a scene reading the wall clock would show
     * something different on a slower machine and could never be
     * asserted at all.
     *
     * The caption is drawn through the 2D renderer after the mesh, since
     * there is one frame and both halves draw into it: what goes last
     * goes in front.
     */
    class SpinScene final
    {
    public:
        /**
         * @brief Work out where the cube has turned to.
         * @param tick How many ticks have been drawn before this one.
         * @return The model matrix for that tick; the identity at tick
         * zero.
         */
        [[nodiscard]] Mat4 modelAt(std::uint64_t tick) const;

        /**
         * @brief Work out what the cube is seen through.
         * @param canvas The area being drawn into, whose proportions
         * the projection follows so the cube is not stretched.
         * @return A camera backed off along +Z, looking at the origin.
         */
        [[nodiscard]] Camera3D cameraFor(Size canvas) const;

        /**
         * @brief Draw one frame.
         * @param flat Clears the frame and draws the caption.
         * @param space Draws the cube.
         * @param cube The uploaded geometry, which must have come from
         * `space`.
         * @param canvas The area being drawn into.
         * @param tick How many ticks have been drawn before this one.
         */
        void draw(
            IRenderer &flat,
            IRenderer3D &space,
            const IMesh &cube,
            Size canvas,
            std::uint64_t tick) const;
    };

} // namespace antwika::gfx3d_demo
