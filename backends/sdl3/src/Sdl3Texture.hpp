#pragma once

#include <SDL3/SDL.h>

#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/Size.hpp>

namespace antwika::gfx::sdl3
{

    class Sdl3Renderer;

    /**
     * @brief Pixels uploaded to one SDL renderer.
     *
     * Registers with the renderer that made it, so that renderer can
     * free the SDL texture while it still has an SDL_Renderer to free it
     * against.
     * Destroying an SDL texture after its renderer has gone is
     * undefined, and a texture is free to outlive its window, so one of
     * the two has to know about the other.
     */
    class Sdl3Texture final : public ITexture
    {
    public:
        /**
         * @brief Construct the texture and register it with its owner.
         * @param owner The renderer that created it, and the only one
         * that may draw it; must outlive this object or call detach().
         * @param texture The SDL texture, owned by this object.
         * @param size The size of the bitmap it was created from.
         */
        Sdl3Texture(
            Sdl3Renderer &owner, SDL_Texture *texture, Size size);

        Sdl3Texture(const Sdl3Texture &) = delete;
        Sdl3Texture(Sdl3Texture &&) = delete;

        Sdl3Texture &operator=(const Sdl3Texture &) = delete;
        Sdl3Texture &operator=(Sdl3Texture &&) = delete;

        /**
         * @brief Free the SDL texture, if the renderer has not already.
         */
        ~Sdl3Texture() override;

        /**
         * @brief Get the size of the bitmap this was created from.
         * @return That size, whatever SDL made of the pixels.
         */
        [[nodiscard]] Size size() const override;

        /**
         * @brief Check which renderer this texture belongs to.
         * @param candidate The renderer proposing to draw it.
         * @return True when candidate is the renderer that created it.
         */
        [[nodiscard]] bool belongsTo(
            const Sdl3Renderer &candidate) const noexcept;

        /**
         * @brief Get the SDL texture to draw.
         * @return The texture, or nullptr once its renderer has gone.
         */
        [[nodiscard]] SDL_Texture *raw() const noexcept;

        /**
         * @brief Give up the SDL texture, which the renderer has freed.
         *
         * Leaves this object valid but drawing nothing, so a caller
         * holding it past its renderer's life is safe rather than sorry.
         */
        void forgetRenderer() noexcept;

    private:
        Sdl3Renderer *owner;
        SDL_Texture *texture;
        Size textureSize;
    };

} // namespace antwika::gfx::sdl3
