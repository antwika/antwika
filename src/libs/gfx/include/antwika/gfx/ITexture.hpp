#pragma once

#include "antwika/gfx/Size.hpp"

namespace antwika::gfx
{

    /**
     * @brief Pixels held by a renderer, ready to be blitted.
     *
     * Opaque on purpose: there is no way to reach the underlying
     * framework object, and no way to read a pixel back.
     * Read-back is the one thing that would let rendering feed the
     * simulation, which IRenderer's write-only projection exists to
     * prevent.
     *
     * A texture belongs to the renderer that created it.
     * Drawing it through any other renderer draws nothing, and
     * destroying it after that renderer's window has closed is safe.
     */
    class ITexture
    {
    public:
        virtual ~ITexture() = default;

        /**
         * @brief Get the size of the pixels this texture holds.
         * @return Exactly the size of the bitmap it was created from,
         * on every backend; one that padded the pixels out internally
         * still reports the size it was given.
         */
        [[nodiscard]] virtual Size size() const = 0;
    };

} // namespace antwika::gfx
