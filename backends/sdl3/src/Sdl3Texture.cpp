#include "Sdl3Texture.hpp"

#include "Sdl3Renderer.hpp"

namespace antwika::gfx::sdl3
{

    Sdl3Texture::Sdl3Texture(
        Sdl3Renderer &owner, SDL_Texture *texture, Size size)
        : owner(&owner),
          texture(texture),
          textureSize(size)
    {
        owner.rememberTexture(*this);
    }

    Sdl3Texture::~Sdl3Texture()
    {
        if (owner != nullptr)
        {
            owner->forgetTexture(*this);
        }

        if (texture != nullptr)
        {
            SDL_DestroyTexture(texture);
        }
    }

    Size Sdl3Texture::size() const
    {
        return textureSize;
    }

    bool Sdl3Texture::belongsTo(
        const Sdl3Renderer &candidate) const noexcept
    {
        return owner == &candidate;
    }

    SDL_Texture *Sdl3Texture::raw() const noexcept
    {
        return texture;
    }

    void Sdl3Texture::forgetRenderer() noexcept
    {
        owner = nullptr;
        texture = nullptr;
    }

}
