#pragma once

#include "antwika/gfx/Size.hpp"

namespace antwika::gfx
{

    struct RenderTargetSpec final
    {
        Size size{1, 1};

        bool depth = false;

        [[nodiscard]] bool operator==(
            const RenderTargetSpec &other) const = default;
    };

}
