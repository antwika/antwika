#include "RaylibTexture.hpp"

#include "RaylibRenderer.hpp"

namespace antwika::gfx::raylib
{

    RaylibTexture::RaylibTexture(
        RaylibRenderer &ownerRenderer,
        ::Texture2D texture,
        Size size,
        const bool owns)
        : RaylibResource(ownerRenderer),
          texture(texture),
          textureSize(size),
          owned(owns)
    {
    }

    RaylibTexture::~RaylibTexture()
    {
        unload();
    }

    Size RaylibTexture::getSize() const
    {
        return textureSize;
    }

    const ::Texture2D &RaylibTexture::getRawHandle() const noexcept
    {
        return texture;
    }

    void RaylibTexture::unloadHandle() noexcept
    {
        if (owned)
        {
            UnloadTexture(texture);
        }
    }

}
