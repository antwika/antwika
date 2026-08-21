#pragma once

#include "antwika/gfx/ITexture.hpp"
#include "antwika/gfx/Size.hpp"

namespace antwika::gfx::fakes
{

    class FakeForeignTexture final : public ITexture
    {
    public:
        [[nodiscard]] Size size() const override
        {
            return Size{.width = 4, .height = 4};
        }
    };

}
