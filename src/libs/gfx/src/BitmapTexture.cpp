#include "BitmapTexture.hpp"

#include <utility>

namespace antwika::gfx::detail
{

    BitmapTexture::BitmapTexture(Bitmap pixels)
        : held(std::move(pixels))
    {
    }

    Size BitmapTexture::size() const
    {
        return held.size;
    }

    const Bitmap &BitmapTexture::image() const noexcept
    {
        return held;
    }

}
