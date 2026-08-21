#pragma once

#include <vector>

#include <antwika/gfx/Rect.hpp>

#include "antwika/ui/HoverPointer.hpp"

namespace antwika::ui
{

    using antwika::gfx::Rect;

    using OccluderRects = std::vector<Rect>;

    [[nodiscard]] bool isOccluded(
        const OccluderRects &occluders, HoverPointer hover) noexcept;

    [[nodiscard]] bool isOccluded(
        const OccluderRects &occluders, const Rect &rect) noexcept;

}
