#include "antwika/autotile/SheetLayout.hpp"

namespace antwika::autotile
{

    namespace
    {
        constexpr std::int32_t kFullMask = 15;
        constexpr std::int32_t kRightColumn = 64;
        constexpr std::int32_t kSpecialRow = 64;

        [[nodiscard]] geometry::Rect halfSlotAt(
            const std::int32_t x, const std::int32_t y) noexcept
        {
            return {.origin = {.x = x, .y = y},
                    .size = {.width = kHalfTile, .height = kHalfTile}};
        }

        [[nodiscard]] geometry::Rect tileSlotAt(
            const std::int32_t x, const std::int32_t y) noexcept
        {
            return {
                .origin = {.x = x, .y = y},
                .size = {
                    .width = kDisplayTile,
                    .height = kDisplayTile}};
        }
    }

    geometry::Rect quadrantSource(const std::uint8_t slot) noexcept
    {
        const auto index = static_cast<std::int32_t>(slot);

        if (index < 8)
        {
            return halfSlotAt(32 + index * kHalfTile, kSpecialRow);
        }

        return halfSlotAt(
            (index - 8) * kHalfTile, kSpecialRow + kHalfTile);
    }

    geometry::Rect sheetSource(
        const TilePiece piece,
        const std::uint8_t mask,
        const std::uint8_t variant) noexcept
    {
        if (piece == TilePiece::Quadrant)
        {
            return quadrantSource(variant);
        }

        if (piece == TilePiece::WallBand)
        {
            return halfSlotAt(0, kSpecialRow);
        }

        if (piece == TilePiece::WallRim)
        {
            return halfSlotAt(kHalfTile, kSpecialRow);
        }

        if (piece == TilePiece::BridgeDeck)
        {
            return halfSlotAt(2 * kHalfTile, kSpecialRow);
        }

        if (piece == TilePiece::Shade)
        {
            return halfSlotAt(3 * kHalfTile, kSpecialRow);
        }

        const auto index = static_cast<std::int32_t>(mask % 16);

        if (index == kFullMask
            && variant >= 1
            && variant <= kWaterFrameBVariant)
        {
            const auto slot =
                static_cast<std::int32_t>(variant) - 1;

            return tileSlotAt(
                kRightColumn + slot % 2 * kDisplayTile,
                slot / 2 * kDisplayTile);
        }

        return tileSlotAt(
            (index % 4) * kDisplayTile,
            (index / 4) * kDisplayTile);
    }

}
