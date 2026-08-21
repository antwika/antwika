#pragma once

#include "antwika/gfx/Size.hpp"
#include "antwika/gfx/WindowId.hpp"

namespace antwika::gfx
{

    struct CloseRequested final
    {
        [[nodiscard]] bool operator==(
            const CloseRequested &other) const = default;
    };

}
