#pragma once

#include "antwika/gfx/Size.hpp"

namespace antwika::gfx
{

    class ITexture
    {
    public:
        virtual ~ITexture() = default;

        [[nodiscard]] virtual Size getSize() const = 0;
    };

}
