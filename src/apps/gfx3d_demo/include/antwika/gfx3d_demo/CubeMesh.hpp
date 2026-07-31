#pragma once

#include <antwika/gfx/MeshData.hpp>

namespace antwika::gfx3d_demo
{

    using antwika::gfx::MeshData;

    /**
     * @brief Build the one-unit cube this demo turns.
     *
     * Four vertices per face rather than eight shared corners, because a
     * corner belongs to three faces that disagree about both the normal
     * and the colour, and a shared vertex could only hold one answer.
     *
     * Every triangle is wound anticlockwise as seen from outside, which
     * is what a backend culling back faces expects; getting it backwards
     * shows the inside of the cube and looks like a lighting bug rather
     * than a winding one.
     *
     * @return The geometry, in the cube's own space, centred on the
     * origin and half a unit from it in each direction.
     */
    [[nodiscard]] MeshData cubeMesh();

} // namespace antwika::gfx3d_demo
