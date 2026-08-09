#pragma once

#include <SDL3/SDL.h>

#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/Size.hpp>

namespace antwika::gfx::sdl3
{

    class Sdl3Renderer;

    class Sdl3Texture final : public ITexture
    {
    public:
        Sdl3Texture(
            Sdl3Renderer &owner, SDL_Texture *texture, Size size);

        Sdl3Texture(const Sdl3Texture &) = delete;
        Sdl3Texture(Sdl3Texture &&) = delete;

        Sdl3Texture &operator=(const Sdl3Texture &) = delete;
        Sdl3Texture &operator=(Sdl3Texture &&) = delete;

        ~Sdl3Texture() override;

        [[nodiscard]] Size size() const override;

        [[nodiscard]] bool belongsTo(
            const Sdl3Renderer &candidate) const noexcept;

        [[nodiscard]] SDL_Texture *raw() const noexcept;

        void forgetRenderer() noexcept;

    private:
        Sdl3Renderer *owner;
        SDL_Texture *texture;
        Size textureSize;
    };

}
