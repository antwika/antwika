#pragma once

#include "antwika/gfx/Rect.hpp"
#include "antwika/gfx/Size.hpp"

namespace antwika::gfx
{

    /**
     * @brief Check whether a blit is one every backend will draw the
     * same way.
     *
     * A source rectangle reaching outside its texture is the one place
     * backends genuinely disagree, so antwika::gfx refuses such a blit
     * rather than letting each of them pick an answer.
     * Every backend asks this before drawing, which is what keeps the
     * rule one function with one set of tests instead of three.
     * A destination is not checked against the drawable area, because
     * clipping at the window's edge is something they all agree on.
     * @param texture The size of the texture being sampled.
     * @param source The region of the texture to take, in its pixels.
     * @param destination The region of the drawable area to fill.
     * @return True when source lies wholly inside the texture and
     * neither rectangle is empty.
     */
    [[nodiscard]] bool blitIsDrawable(
        Size texture, Rect source, Rect destination) noexcept;

} // namespace antwika::gfx
