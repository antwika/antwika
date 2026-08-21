#include "NullTexture.hpp"

namespace antwika::gfx::detail
{

    NullTexture::NullTexture(Size size)
        : textureSize(size)
    {
    }

    Size NullTexture::size() const
    {
        return textureSize;
    }

}
