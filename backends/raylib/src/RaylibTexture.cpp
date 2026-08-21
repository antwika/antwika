#include "RaylibTexture.hpp"

#include "RaylibRenderer.hpp"

namespace antwika::gfx::raylib
{

    RaylibTexture::RaylibTexture(
        RaylibRenderer &ownerRenderer,
        ::Texture2D texture,
        Size size,
        const bool owns)
        : owner(&ownerRenderer),
          texture(texture),
          textureSize(size),
          owned(owns)
    {
        ownerRenderer.trackTexture(*this);
    }

    RaylibTexture::~RaylibTexture()
    {
        if (owner != nullptr)
        {
            owner->untrackTexture(*this);
        }

        if (loaded && owned)
        {
            UnloadTexture(texture);
        }
    }

    Size RaylibTexture::size() const
    {
        return textureSize;
    }

    bool RaylibTexture::isOwnedBy(
        const RaylibRenderer &candidateRenderer) const noexcept
    {
        return owner == &candidateRenderer;
    }

    const ::Texture2D &RaylibTexture::raw() const noexcept
    {
        return texture;
    }

    bool RaylibTexture::isLoaded() const noexcept
    {
        return loaded;
    }

    bool RaylibTexture::isOwned() const noexcept
    {
        return owned;
    }

    void RaylibTexture::untrackRenderer() noexcept
    {
        owner = nullptr;
        loaded = false;
    }

}
