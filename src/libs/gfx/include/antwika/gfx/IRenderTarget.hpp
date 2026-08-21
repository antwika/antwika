#pragma once

#include "antwika/gfx/ITexture.hpp"
#include "antwika/gfx/Size.hpp"

namespace antwika::gfx
{

    class IRenderTarget
    {
    public:
        virtual ~IRenderTarget() = default;

        [[nodiscard]] virtual Size size() const = 0;

        [[nodiscard]] virtual const ITexture *color() const = 0;

        [[nodiscard]] virtual const ITexture *depth() const = 0;
    };

}
