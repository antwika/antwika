#pragma once

#include <cstdint>

namespace antwika::autotile
{

    enum class TilePiece : std::uint8_t
    {
        Surface = 0,
        WallRim,
        WallBand,
        BridgeDeck,
        Shade,
    };

    [[nodiscard]] constexpr TilePiece enumBound(TilePiece) noexcept
    {
        return TilePiece::Shade;
    }

}
