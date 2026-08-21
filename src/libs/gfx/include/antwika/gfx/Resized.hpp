#pragma once

#include "antwika/gfx/Size.hpp"
#include "antwika/gfx/WindowId.hpp"

namespace antwika::gfx
{

    struct Resized final
    {
        Size size;

        [[nodiscard]] bool operator==(const Resized &other) const = default;
    };

}
