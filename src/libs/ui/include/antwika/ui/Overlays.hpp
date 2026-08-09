#pragma once

#include <vector>

#include <antwika/gfx/Rect.hpp>

#include "antwika/ui/HoverPointer.hpp"

namespace antwika::ui
{

    using antwika::gfx::Rect;

    using Overlays = std::vector<Rect>;

    [[nodiscard]] bool overlaid(
        const Overlays &overlays, HoverPointer hover) noexcept;

}
