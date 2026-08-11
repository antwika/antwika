#pragma once

#include <antwika/geometry/Rect.hpp>

#include "antwika/autotile/TileDraw.hpp"

namespace antwika::autotile
{

    /**
     * @brief Locates a system piece on the 32x8 system sheet.
     *
     * @param kind The system draw kind to locate.
     * @return The 8x8 source rectangle of that kind.
     *
     * Requires: kind is not DrawKind::Sprite, because sprite draws
     *           source from a tileset atlas instead.
     * Ensures:  WallBand, WallRim, BridgeDeck and Shade map to the
     *           sheet's four slots in that order, left to right.
     */
    [[nodiscard]] geometry::Rect systemSource(DrawKind kind) noexcept;

}
