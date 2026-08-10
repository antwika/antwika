#include "antwika/autotile/SheetLayout.hpp"

namespace antwika::autotile
{

    namespace
    {
        constexpr std::int32_t kFullMask = 15;
        constexpr std::int32_t kSpecialRow = 32;
        constexpr std::int32_t kVariantRow = 40;
        constexpr std::int32_t kVariantSlots = 3;

        [[nodiscard]] geometry::Rect slotAt(
            const std::int32_t x, const std::int32_t y) noexcept
        {
            return {.origin = {.x = x, .y = y},
                    .size = {.width = kHalfTile, .height = kHalfTile}};
        }
    }

    geometry::Rect sheetSource(
        const TilePiece piece,
        const std::uint8_t mask,
        const std::uint8_t variant) noexcept
    {
        if (piece == TilePiece::WallBand)
        {
            return slotAt(0, kSpecialRow);
        }

        if (piece == TilePiece::WallRim)
        {
            return slotAt(kHalfTile, kSpecialRow);
        }

        if (piece == TilePiece::BridgeDeck)
        {
            return slotAt(2 * kHalfTile, kSpecialRow);
        }

        if (piece == TilePiece::Shade)
        {
            return slotAt(3 * kHalfTile, kSpecialRow);
        }

        const auto index = static_cast<std::int32_t>(mask % 16);

        if (index == kFullMask
            && variant >= 1
            && variant <= kVariantSlots)
        {
            return slotAt((variant - 1) * kHalfTile, kVariantRow);
        }

        return slotAt(
            (index % 4) * kHalfTile, (index / 4) * kHalfTile);
    }

}
