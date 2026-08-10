#pragma once

#include <cstdint>

#include <antwika/geometry/Rect.hpp>

#include "antwika/autotile/TilePiece.hpp"

namespace antwika::autotile
{

    inline constexpr std::int32_t kHalfTile = 8;
    inline constexpr std::int32_t kUnit = 16;
    inline constexpr std::int32_t kLevelRise = 8;
    inline constexpr std::int32_t kDisplayTile = 16;

    inline constexpr std::uint32_t kSheetWidth = 96;
    inline constexpr std::uint32_t kSheetHeight = 64;

    /**
     * @brief Locates a piece's sprite on the terrain sheet.
     *
     * @param piece The piece whose sprite is wanted.
     * @param mask The corner mask; only the low four bits are read.
     * @param variant The alternate-art index; a variant no slot backs
     *                falls back to the plain mask tile.
     * @return The source rectangle on the sheet: 16x16 for Surface
     *         pieces and 8x8 for the special half-tile pieces.
     *
     * Ensures: every input yields a rectangle inside the sheet.
     */
    [[nodiscard]] geometry::Rect sheetSource(
        TilePiece piece,
        std::uint8_t mask,
        std::uint8_t variant = 0) noexcept;

}
