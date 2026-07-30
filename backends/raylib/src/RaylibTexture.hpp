#pragma once

#include <raylib.h>

#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/Size.hpp>

namespace antwika::gfx::raylib
{

    class RaylibRenderer;

    /**
     * @brief Pixels uploaded to raylib's one GL context.
     *
     * Registers with the renderer that made it, so that renderer can
     * unload it while the context is still alive.
     * raylib frees a texture through the context CloseWindow() destroys,
     * and a texture is free to outlive its window, so one of the two has
     * to know about the other.
     *
     * This is the only header naming a raylib type, which is why
     * RaylibRenderer forward-declares this class rather than including
     * it.
     */
    class RaylibTexture final : public ITexture
    {
    public:
        /**
         * @brief Construct the texture and register it with its owner.
         * @param owner The renderer that created it, and the only one
         * that may draw it; must outlive this object or call detach().
         * @param texture The loaded raylib texture, owned by this
         * object.
         * @param size The size of the bitmap it was created from.
         */
        RaylibTexture(
            RaylibRenderer &owner, ::Texture2D texture, Size size);

        RaylibTexture(const RaylibTexture &) = delete;
        RaylibTexture(RaylibTexture &&) = delete;

        RaylibTexture &operator=(const RaylibTexture &) = delete;
        RaylibTexture &operator=(RaylibTexture &&) = delete;

        /**
         * @brief Unload the texture, if the renderer has not already.
         */
        ~RaylibTexture() override;

        /**
         * @brief Get the size of the bitmap this was created from.
         * @return That size, whatever raylib made of the pixels.
         */
        [[nodiscard]] Size size() const override;

        /**
         * @brief Check which renderer this texture belongs to.
         * @param candidate The renderer proposing to draw it.
         * @return True when candidate is the renderer that created it.
         */
        [[nodiscard]] bool belongsTo(
            const RaylibRenderer &candidate) const noexcept;

        /**
         * @brief Get the raylib texture to draw.
         * @return The texture; only meaningful while loaded.
         */
        [[nodiscard]] const ::Texture2D &raw() const noexcept;

        /**
         * @brief Check whether this texture is still on the GPU.
         * @return False once its renderer has unloaded it.
         */
        [[nodiscard]] bool isLoaded() const noexcept;

        /**
         * @brief Give up the texture, which the renderer has unloaded.
         *
         * Leaves this object valid but drawing nothing, so a caller
         * holding it past its renderer's life is safe rather than sorry.
         */
        void forgetRenderer() noexcept;

    private:
        RaylibRenderer *owner;
        ::Texture2D texture;
        Size textureSize;
        bool loaded = true;
    };

} // namespace antwika::gfx::raylib
