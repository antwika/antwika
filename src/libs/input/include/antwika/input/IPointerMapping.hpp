#pragma once

#include "antwika/input/Position.hpp"

namespace antwika::input
{

    /**
     * @brief Reads a position a device reported as a position on the
     * surface an application lays itself out against.
     *
     * Position.hpp already says the two need not be the same thing: a
     * device reports in its backend's own surface coordinates, and what
     * those are measured against is nobody's business here. An
     * application drawing a fixed-size picture into a window of some
     * other size is where the two really do differ, and this is the seam
     * it says so through.
     *
     * **It is deliberately expressed with no gfx type at all**, so this
     * library goes on naming no window and no size. What the mapping is
     * lives above both libraries, in antwika::app, where a window and a
     * canvas can be named in the same sentence.
     *
     * An implementation must be a pure function of its own state and of
     * the position handed to it, and must be integer throughout: what
     * comes back is recorded input, so a value differing in its last bit
     * between two machines is a divergent replay rather than a
     * misplaced pixel.
     */
    class IPointerMapping
    {
    public:
        virtual ~IPointerMapping() = default;

        /**
         * @brief Read a device position as a surface position.
         * @param position Where the device said the pointer was.
         * @return Where that is on the surface the application uses.
         */
        [[nodiscard]] virtual Position toSurface(
            Position position) const = 0;
    };

} // namespace antwika::input
