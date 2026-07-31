#pragma once

#include <cstdint>

#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/BuildTool.hpp"
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
    inline constexpr std::uint32_t kAtlasRows = 4;

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
     * @brief The slot holding the first building.
     */
    inline constexpr std::uint32_t kFirstBuildingSlot =
        kFirstWalkerSlot + kDirectionCount;

    /**
     * @brief How many building tiles there are, one per building tool.
     *
     * The static_assert below is what keeps this and kBuildingKindCount
     * from disagreeing.
     */
    inline constexpr std::uint32_t kBuildingSlotCount = 5;

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

    // Every number above is constexpr, so a wrong layout can fail here.
    // On screen is the only other place it could fail.

    // atlasSlot() wraps a slot round rather than rejecting it.
    // That is safe only while every derived slot is one the atlas has.
    // buildingTile() derives the highest of them.
    static_assert(
        kFirstBuildingSlot + kBuildingSlotCount
            <= kAtlasColumns * kAtlasRows,
        "the atlas has no room for every building slot");

    // One building tile per tool that places one.
    // Adding a tool without drawing it would show somebody else's art.
    static_assert(
        kBuildingSlotCount == kBuildingKindCount,
        "there must be a building tile for every building tool");

    // kLinkMask is built from the four directions this file names.
    // A fifth enumerator would raise kDirectionCount past that mask.
    // linkBit() would then hand out a bit no road tile has.
    static_assert(
        kLinkMask == (1U << kDirectionCount) - 1U,
        "kDirectionCount must count exactly the named directions");

    // One road tile per link mask is what makes roadTile() a lookup.
    static_assert(
        kRoadSlotCount == 1U << kDirectionCount,
        "there must be a road tile for every link mask");

    // The art is kAtlasColumns by kAtlasRows tiles.
    // kAtlasSize is written that way above.
    // Saying it again is what makes a hand-typed size a build error.
    static_assert(
        kAtlasSize.width == kAtlasColumns * kAtlasTileSize.width
            && kAtlasSize.height == kAtlasRows * kAtlasTileSize.height,
        "kAtlasSize must be the grid of tiles the art is drawn on");

    /**
     * @brief Get where a slot's tile is in the atlas.
     *
     * Arithmetic over a slot number rather than a table of rectangles, so
     * there is no list here that could disagree with the picture.
     *
     * **This header is the address map, and the PNG beside it is the
     * art.** The picture is drawn and curated rather than generated, so
     * repainting a tile is free and nothing has to be re-run; what is
     * *not* free is moving one, because these numbers are what decide
     * which pixels a slot means. See docs/game-texture-atlas.md.
     *
     * The slots run the ground, then the sixteen roads in link-mask
     * order, then the four walkers in Direction order, then the
     * buildings in BuildingKind order. A road's mask indexing its slot
     * is what makes a junction's art a lookup rather than four
     * decisions, and the mask's bits are the Direction enumerators' own
     * indices, handed out by linkBit().
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

    /**
     * @brief Get the tile a building of this kind is drawn from.
     * @param kind The building's kind.
     * @return The building tile's rectangle, in atlas pixels.
     */
    [[nodiscard]] constexpr Rect buildingTile(BuildingKind kind) noexcept
    {
        return atlasSlot(
            kFirstBuildingSlot
            + static_cast<std::uint32_t>(
                buildingKindIndex(kind) % kBuildingSlotCount));
    }

    /**
     * @brief Get the tile a tool's placement would be drawn from.
     *
     * The one place the palette's choice becomes art, so the ghost and
     * the thing it stands for cannot come from two different decisions.
     *
     * @param tool The selected tool.
     * @param links Which neighbours a road would run to; ignored for
     * every other tool.
     * @return The tile's rectangle, in atlas pixels.
     */
    [[nodiscard]] constexpr Rect toolTile(
        BuildTool tool, std::uint8_t links) noexcept
    {
        const auto kind = buildingKindOf(tool);

        return kind.has_value() ? buildingTile(*kind) : roadTile(links);
    }

} // namespace antwika::game
