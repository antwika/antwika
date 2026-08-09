#include "RaylibTexture.hpp"

#include "RaylibRenderer.hpp"

namespace antwika::gfx::raylib
{

    RaylibTexture::RaylibTexture(
        RaylibRenderer &owner, ::Texture2D texture, Size size)
        : owner(&owner),
          texture(texture),
          textureSize(size)
    {
        owner.rememberTexture(*this);
    }

    RaylibTexture::~RaylibTexture()
    {
        if (owner != nullptr)
        {
            owner->forgetTexture(*this);
        }

        if (loaded)
        {
            UnloadTexture(texture);
        }
    }

    Size RaylibTexture::size() const
    {
        return textureSize;
    }

    bool RaylibTexture::belongsTo(
        const RaylibRenderer &candidate) const noexcept
    {
        return owner == &candidate;
    }

    const ::Texture2D &RaylibTexture::raw() const noexcept
    {
        return texture;
    }

    bool RaylibTexture::isLoaded() const noexcept
    {
        return loaded;
    }

    void RaylibTexture::forgetRenderer() noexcept
    {
        owner = nullptr;
        loaded = false;
    }

}
