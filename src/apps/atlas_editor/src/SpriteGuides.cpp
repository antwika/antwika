#include "antwika/atlas_editor/SpriteGuides.hpp"

#include <cstdint>
#include <optional>

namespace antwika::atlas_editor
{

    std::optional<SpriteGuides> guidesForTile(const TileGrid tile) noexcept
    {
        constexpr std::uint32_t kMargins = kSpriteSideMargin * 2;
        constexpr std::uint32_t kBands =
            kSpriteHeadroom + kSpriteSkirtBand;

        // Wider than its margins and taller than its two bands.
        // A slot that is not leaves nothing for a diamond to be.
        // Checked before the subtractions rather than after them.
        // Unsigned arithmetic would wrap to an enormous diamond.
        if (tile.width <= kMargins || tile.height <= kBands)
        {
            return std::nullopt;
        }

        const std::uint32_t width = tile.width - kMargins;
        const std::uint32_t height = tile.height - kBands;

        // The two derivations have to agree, and this is where.
        // The margins and the bands leave two separate numbers.
        // Only an isometric diamond makes those two one shape.
        if (width != height * 2)
        {
            return std::nullopt;
        }

        return SpriteGuides{
            .pivot =
                {.x = static_cast<std::int32_t>(tile.width / 2),
                 .y = static_cast<std::int32_t>(kSpriteHeadroom + height)},
            .footprint = {.width = width, .height = height}};
    }

} // namespace antwika::atlas_editor
