#pragma once

#include <cstdint>

#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/game/Direction.hpp"

namespace antwika::game
{

    using antwika::gfx::Rect;
    using antwika::gfx::Size;

    /**
     * @brief The size of one tile's cell in the atlas, in pixels.
     *
     * The bounding box of a diamond at the closest zoom -- twice
     * kZoomHalfWidths' last entry by that entry -- so every blit scales
     * the art down rather than up.
     */
    inline constexpr Size kAtlasTileSize{.width = 128, .height = 64};

    /**
     * @brief How many tiles a row of the atlas holds.
     */
    inline constexpr std::uint32_t kAtlasColumns = 8;

    /**
     * @brief How many rows of tiles the atlas holds.
     */
    inline constexpr std::uint32_t kAtlasRows = 3;

    /**
     * @brief The size the atlas image must be, in pixels.
     */
    inline constexpr Size kAtlasSize{
        .width = kAtlasColumns * kAtlasTileSize.width,
        .height = kAtlasRows * kAtlasTileSize.height};

    /**
     * @brief The slot holding the plain ground tile.
     */
    inline constexpr std::uint32_t kGroundSlot = 0;

    /**
     * @brief The slot holding the road with no links at all.
     */
    inline constexpr std::uint32_t kFirstRoadSlot = 1;

    /**
     * @brief How many road tiles there are, one per link mask.
     */
    inline constexpr std::uint32_t kRoadSlotCount = 16;

    /**
     * @brief The slot holding the walker facing north.
     */
    inline constexpr std::uint32_t kFirstWalkerSlot =
        kFirstRoadSlot + kRoadSlotCount;

    /**
     * @brief Get which bit of a link mask stands for one direction.
     *
     * The direction's own index, so a mask and a Direction cannot drift
     * apart -- adding a fifth direction would move both at once.
     *
     * @param direction The direction to ask about.
     * @return That direction's bit, with no other set.
     */
    [[nodiscard]] constexpr std::uint8_t linkBit(
        Direction direction) noexcept
    {
        return static_cast<std::uint8_t>(
            1U << (directionIndex(direction) % kDirectionCount));
    }

    /**
     * @brief The bits of a link mask that name a direction.
     */
    inline constexpr std::uint8_t kLinkMask =
        linkBit(Direction::North) | linkBit(Direction::East)
        | linkBit(Direction::South) | linkBit(Direction::West);

    /**
     * @brief Get where a slot's tile is in the atlas.
     *
     * Arithmetic over a slot number rather than a table of rectangles, so
     * there is no list here that could disagree with the picture.
     *
     * The picture is drawn by scripts/generate_game_atlas.py, which
     * parses the constants above out of this header rather than restating
     * them -- so these really are the same numbers, and moving one moves
     * the art with it. It matches them by name and by shape, so renaming
     * or rewriting one of the declarations above fails the generator
     * loudly instead of drifting the picture quietly.
     *
     * A road's bit ordering travels the same way: the generator reads the
     * Direction enumerators in declaration order, since that is what
     * linkBit() shifts by.
     *
     * The slots run the ground, then the sixteen roads in link-mask
     * order, then the four walkers in Direction order. A road's mask
     * indexing its own slot is what makes a junction's art a lookup
     * rather than four decisions.
     *
     * @param slot The slot to place; one past the last wraps round rather
     * than being rejected, since every caller here derives its own.
     * @return The tile's rectangle, in atlas pixels.
     */
    [[nodiscard]] constexpr Rect atlasSlot(std::uint32_t slot) noexcept
    {
        const auto wrapped = slot % (kAtlasColumns * kAtlasRows);

        return Rect{
            .origin =
                {.x = static_cast<std::int32_t>(
                     (wrapped % kAtlasColumns) * kAtlasTileSize.width),
                 .y = static_cast<std::int32_t>(
                     (wrapped / kAtlasColumns) * kAtlasTileSize.height)},
            .size = kAtlasTileSize};
    }

    /**
     * @brief Get the tile a cell with nothing on it is drawn from.
     * @return The ground tile's rectangle, in atlas pixels.
     */
    [[nodiscard]] constexpr Rect groundTile() noexcept
    {
        return atlasSlot(kGroundSlot);
    }

    /**
     * @brief Get the tile a road with these links is drawn from.
     * @param links Which neighbours the road runs to, as linkBit() bits;
     * anything outside kLinkMask is ignored.
     * @return The road tile's rectangle, in atlas pixels.
     */
    [[nodiscard]] constexpr Rect roadTile(std::uint8_t links) noexcept
    {
        return atlasSlot(kFirstRoadSlot + (links & kLinkMask));
    }

    /**
     * @brief Get the tile a walker facing this way is drawn from.
     * @param facing The direction the walker is facing.
     * @return The walker tile's rectangle, in atlas pixels.
     */
    [[nodiscard]] constexpr Rect walkerTile(Direction facing) noexcept
    {
        return atlasSlot(
            kFirstWalkerSlot
            + static_cast<std::uint32_t>(
                directionIndex(facing) % kDirectionCount));
    }

} // namespace antwika::game
