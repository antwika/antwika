#pragma once

#include "antwika/gfx/ITexture.hpp"
#include "antwika/gfx/Size.hpp"

namespace antwika::gfx::detail
{

    /**
     * @brief Texture that remembers its size and holds no pixels.
     *
     * The headless counterpart to a real backend's texture: it is
     * created, reported on and destroyed exactly like one, so the same
     * application code runs with no display and no framework.
     */
    class NullTexture final : public ITexture
    {
    public:
        /**
         * @brief Construct the texture.
         * @param size The size of the bitmap it stands in for.
         */
        explicit NullTexture(Size size);

        NullTexture(const NullTexture &) = delete;
        NullTexture(NullTexture &&) = delete;

        NullTexture &operator=(const NullTexture &) = delete;
        NullTexture &operator=(NullTexture &&) = delete;

        /**
         * @brief Get the size this texture was created with.
         * @return That size, unchanged.
         */
        [[nodiscard]] Size size() const override;

    private:
        Size textureSize;
    };

} // namespace antwika::gfx::detail
