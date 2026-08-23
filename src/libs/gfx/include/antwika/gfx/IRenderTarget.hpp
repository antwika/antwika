#pragma once

#include "antwika/gfx/ITexture.hpp"
#include "antwika/gfx/Size.hpp"

namespace antwika::gfx
{

    class IRenderTarget
    {
    public:
        virtual ~IRenderTarget() = default;

        [[nodiscard]] virtual Size getSize() const = 0;

        [[nodiscard]] virtual const ITexture *getColor() const = 0;

        [[nodiscard]] virtual const ITexture *getDepth() const = 0;
    };

}
