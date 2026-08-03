#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/game/BuildTool.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/Ruin.hpp"

namespace antwika::game
{

    using antwika::gfx::Point;
    using antwika::gfx::Rect;
    using antwika::gfx::Size;

    /**
     * @brief The size of one grid cell's diamond in art pixels.
     *
     * Every sheet draws its footprint to this one iso tile, so a sprite
     * from any of them lands on the same grid.  The camera's zoom table
     * scales it; kZoomHalfWidths' middle entry is where art pixels and
     * screen pixels are one to one.
     */
    inline constexpr Size kIsoTileSize{.width = 32, .height = 16};

    /**
     * @brief Which of the three sheets a sprite is blitted from.
     *
     * One sheet per footprint edge, because a bigger block wants a
     * bigger picture: a sprite's diamond grows with the block it stands
     * on while its margins and headroom stay the same, so one sheet of
     * one sprite size cannot hold all three without wasting most of it.
     *
     * Values are contiguous from zero, so a kind can index a table --
     * and they are one less than the footprint edge they draw, which is
     * what lets buildingAtlasOf() be arithmetic rather than a switch.
     */
    enum class AtlasKind : std::uint8_t
    {
        OneByOne = 0,  ///< Ground, roads, walkers and 1x1 buildings.
        TwoByTwo,      ///< 2x2 buildings.
        ThreeByThree,  ///< 3x3 buildings.
    };

    /**
     * @brief How many sheets there are.
     */
    inline constexpr std::size_t kAtlasKindCount =
        static_cast<std::size_t>(AtlasKind::ThreeByThree) + 1;

    /**
     * @brief Get a sheet's index, for addressing a per-sheet table.
     * @param kind The sheet to index.
     * @return The index, always below kAtlasKindCount for a named sheet.
     */
    [[nodiscard]] constexpr std::size_t atlasKindIndex(
        AtlasKind kind) noexcept
    {
        return static_cast<std::size_t>(kind);
    }

    /**
     * @brief How many sprites a row of any sheet holds.
     */
    inline constexpr std::uint32_t kAtlasColumns = 8;

    /**
     * @brief How many rows of sprites any sheet holds.
     */
    inline constexpr std::uint32_t kAtlasRows = 8;

    /**
     * @brief One sheet's geometry: its sprite size and where the grid is.
     *
     * The pivot is the bottom corner of the sprite's footprint diamond,
     * in sprite pixels.  Blitting is anchoring that point to the block's
     * own bottom corner on screen, which is the whole of how a sprite
     * taller than its diamond stays on its cell: headroom rises above
     * the anchor and the base block's skirt hangs below it.
     */
    struct AtlasSpec
    {
        /** @brief One sprite's cell in the sheet, in pixels. */
        Size spriteSize;

        /** @brief The footprint diamond's bottom corner, in the cell. */
        Point pivot;
    };

    /**
     * @brief Each sheet's geometry, indexed by AtlasKind.
     *
     * Every entry is the same shape said three sizes: 16 pixels of
     * margin either side of the diamond, 48 above its top corner for
     * headroom, and 32 below its bottom corner for the base block's
     * skirt and its padding.
     */
    inline constexpr std::array<AtlasSpec, kAtlasKindCount> kAtlasSpecs{{
        {.spriteSize = {.width = 64, .height = 96},
         .pivot = {.x = 32, .y = 64}},
        {.spriteSize = {.width = 96, .height = 112},
         .pivot = {.x = 48, .y = 80}},
        {.spriteSize = {.width = 128, .height = 128},
         .pivot = {.x = 64, .y = 96}},
    }};

    /**
     * @brief Get one sheet's geometry.
     * @param kind The sheet to describe.
     * @return Its sprite size and pivot.
     */
    [[nodiscard]] constexpr AtlasSpec atlasSpec(AtlasKind kind) noexcept
    {
        return kAtlasSpecs[atlasKindIndex(kind) % kAtlasKindCount];
    }

    /**
     * @brief Get the size a sheet's image must be, in pixels.
     * @param kind The sheet to size.
     * @return Its grid of sprites, in pixels.
     */
    [[nodiscard]] constexpr Size atlasSizeOf(AtlasKind kind) noexcept
    {
        const auto spec = atlasSpec(kind);

        return Size{
            .width = kAtlasColumns * spec.spriteSize.width,
            .height = kAtlasRows * spec.spriteSize.height};
    }

    /**
     * @brief Get where a sprite is in its sheet.
     *
     * Arithmetic over a sprite index rather than a table of rectangles,
     * so there is no list here that could disagree with the picture.
     *
     * **This header is the address map, and the PNGs beside it are the
     * art.** The pictures are drawn and curated rather than generated,
     * so repainting a sprite is free and nothing has to be re-run; what
     * is *not* free is moving one, because these numbers are what decide
     * which pixels an index means.  See wiki/apps/game-texture-atlas.md.
     *
     * @param kind The sheet the sprite is in.
     * @param index The sprite to place; one past the last wraps round
     * rather than being rejected, since every caller here derives its
     * own.
     * @return The sprite's rectangle, in that sheet's pixels.
     */
    [[nodiscard]] constexpr Rect spriteRect(
        AtlasKind kind, std::uint32_t index) noexcept
    {
        const auto spec = atlasSpec(kind);
        const auto wrapped = index % (kAtlasColumns * kAtlasRows);

        return Rect{
            .origin =
                {.x = static_cast<std::int32_t>(
                     (wrapped % kAtlasColumns) * spec.spriteSize.width),
                 .y = static_cast<std::int32_t>(
                     (wrapped / kAtlasColumns) * spec.spriteSize.height)},
            .size = spec.spriteSize};
    }

    /**
     * @brief The 1x1 sprite holding the plain ground tile.
     */
    inline constexpr std::uint32_t kGroundSprite = 0;

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
     * @brief How many road sprites there are, one per link mask.
     */
    inline constexpr std::uint32_t kRoadSpriteCount = 16;

    /**
     * @brief Which 1x1 sprite draws a road with each set of links.
     *
     * **A table rather than `1 + mask`, because the sheet orders its
     * junctions by arm count and the mask does not.** The index is the
     * mask -- north is 1, east is 2, south is 4, west is 8, the bits
     * being the Direction enumerators' own indices handed out by
     * linkBit() -- and the entry is the sprite the sheet drew for that
     * junction.
     *
     * The sheet names its arms in screen directions, and the projection
     * shears grid onto screen the one fixed way: a road running to the
     * north neighbour shows a north-east arm on screen, east shows
     * south-east, south shows south-west and west shows north-west.
     */
    inline constexpr std::array<std::uint8_t, kRoadSpriteCount>
        kRoadSpriteByLinks{
            1,   // -        the road that joins nothing
            2,   // N        one arm, north-east on screen
            3,   // E        one arm, south-east on screen
            6,   // N+E      the corner north-east and south-east
            4,   // S        one arm, south-west on screen
            15,  // N+S      the straight north-east to south-west
            7,   // E+S      the corner south-east and south-west
            10,  // N+E+S    the tee missing its north-west arm
            5,   // W        one arm, north-west on screen
            9,   // N+W      the corner north-west and north-east
            14,  // E+W      the straight south-east to north-west
            13,  // N+E+W    the tee missing its south-west arm
            8,   // S+W      the corner south-west and north-west
            12,  // N+S+W    the tee missing its south-east arm
            11,  // E+S+W    the tee missing its north-east arm
            16,  // N+E+S+W  the four-way crossing
        };

    /**
     * @brief The first of the four walker rows in the 1x1 sheet.
     *
     * A whole row per facing rather than a sprite, in Direction order,
     * with the walk cycle's frames in the first kWalkCycleFrames
     * columns and the rest of each row still reserved.
     */
    inline constexpr std::uint32_t kFirstWalkerRow = 3;

    /**
     * @brief How many frames each facing's walk cycle has.
     *
     * The first kWalkCycleFrames columns of a facing's row, cycled
     * left to right; column 0 doubles as the standing frame, so an
     * idle walker is the cycle held at its start rather than a fifth
     * sprite.
     */
    inline constexpr std::uint32_t kWalkCycleFrames = 4;

    /**
     * @brief Which sprite draws each building kind, in its own sheet.
     *
     * Indexed by BuildingKind; which sheet each entry counts in is
     * buildingAtlasOf(), so 0 here means the farm for a 2x2 kind and
     * the storehouse for the 3x3 one.
     */
    inline constexpr std::array<std::uint8_t, kBuildingKindCount>
        kBuildingSprites{
            17,  // House         1x1
            0,   // Farm          2x2
            1,   // ClayPit       2x2
            2,   // Workshop      2x2
            0,   // Storage       3x3
            3,   // Market        2x2
            18,  // Well          1x1
            19,  // Doctor        1x1
            20,  // FireStation   1x1
            21,  // EngineerPost  1x1
        };

    /**
     * @brief Which sprite draws debris, per sheet.
     *
     * One entry per AtlasKind rather than per BuildingKind, because
     * debris is the same picture whatever stood there -- what varies
     * is the size, and the sheet already says that.
     */
    inline constexpr std::array<std::uint8_t, kAtlasKindCount>
        kDebrisSprites{22, 8, 8};

    /**
     * @brief Which sprite draws a building on fire, per sheet.
     *
     * Beside its debris in every sheet, so the pair reads as one
     * story in the art.
     */
    inline constexpr std::array<std::uint8_t, kAtlasKindCount>
        kFireSprites{23, 9, 9};

    /**
     * @brief Get which sheet a building kind is drawn from.
     *
     * Derived from the kind's footprint rather than tabled beside the
     * sprite index, so a building cannot claim a sheet whose diamond its
     * block does not fit -- a 2x2 kind is in the 2x2 sheet by
     * construction.
     *
     * @param kind The building's kind.
     * @return The sheet whose diamond matches its footprint.
     */
    [[nodiscard]] constexpr AtlasKind buildingAtlasOf(
        BuildingKind kind) noexcept
    {
        return static_cast<AtlasKind>(footprintOf(kind).width - 1);
    }

    // Every number above is constexpr, so a wrong layout can fail here.
    // On screen is the only other place it could fail.

    // The road table must name each of the sixteen junction sprites once.
    // A repeated entry is two masks drawing one junction.
    // And the sprite it crowded out would never be drawn at all.
    static_assert(
        []
        {
            std::array<bool, kRoadSpriteCount> seen{};

            for (const auto sprite : kRoadSpriteByLinks)
            {
                if (sprite < 1 || sprite > kRoadSpriteCount
                    || seen[sprite - 1])
                {
                    return false;
                }

                seen[sprite - 1] = true;
            }

            return true;
        }(),
        "the road table must be a permutation of sprites 1 to 16");

    // kLinkMask is built from the four directions this file names.
    // A fifth enumerator would raise kDirectionCount past that mask.
    // linkBit() would then hand out a bit no road sprite has.
    static_assert(
        kLinkMask == (1U << kDirectionCount) - 1U,
        "kDirectionCount must count exactly the named directions");

    // One road sprite per link mask is what makes roadTile() a lookup.
    static_assert(
        kRoadSpriteCount == 1U << kDirectionCount,
        "there must be a road sprite for every link mask");

    // The last walker row has to exist in the sheet.
    static_assert(
        kFirstWalkerRow + kDirectionCount <= kAtlasRows,
        "the 1x1 sheet has no row for every walker facing");

    // A cycle wider than a row would walk into the next facing's art.
    static_assert(
        kWalkCycleFrames <= kAtlasColumns,
        "a facing's walk cycle must fit in its own row");

    // A footprint's edge is what picks a sheet, so it must pick one.
    static_assert(
        []
        {
            for (std::size_t index = 0; index < kBuildingKindCount;
                 ++index)
            {
                const auto width =
                    footprintOf(static_cast<BuildingKind>(index)).width;

                if (width < 1
                    || width > static_cast<std::int32_t>(kAtlasKindCount))
                {
                    return false;
                }
            }

            return true;
        }(),
        "every footprint edge must name a sheet of its own size");

    // Two kinds sharing a sheet must not share a sprite.
    // The building the palette placed would wear somebody else's art.
    static_assert(
        []
        {
            for (std::size_t a = 0; a < kBuildingKindCount; ++a)
            {
                for (std::size_t b = a + 1; b < kBuildingKindCount; ++b)
                {
                    const auto kindA = static_cast<BuildingKind>(a);
                    const auto kindB = static_cast<BuildingKind>(b);

                    if (buildingAtlasOf(kindA) == buildingAtlasOf(kindB)
                        && kBuildingSprites[a] == kBuildingSprites[b])
                    {
                        return false;
                    }
                }
            }

            return true;
        }(),
        "two kinds in one sheet must not share a sprite");

    // Every named sprite index has to exist in an 8 by 8 sheet.
    // spriteRect() wraps an index round rather than rejecting it.
    // That is safe only while every derived index is one a sheet has.
    static_assert(
        []
        {
            for (const auto sprite : kBuildingSprites)
            {
                if (sprite >= kAtlasColumns * kAtlasRows)
                {
                    return false;
                }
            }

            return true;
        }(),
        "a building sprite index must be inside its sheet");

    // The ruin sprites live by the same rules as the building ones.
    // Inside the sheet, and distinct from each other.
    // And in no slot a building sprite already owns.
    static_assert(
        []
        {
            for (std::size_t sheet = 0; sheet < kAtlasKindCount; ++sheet)
            {
                const auto debris = kDebrisSprites[sheet];
                const auto fire = kFireSprites[sheet];

                if (debris >= kAtlasColumns * kAtlasRows
                    || fire >= kAtlasColumns * kAtlasRows
                    || debris == fire)
                {
                    return false;
                }

                for (std::size_t kind = 0; kind < kBuildingKindCount;
                     ++kind)
                {
                    const auto owned =
                        buildingAtlasOf(static_cast<BuildingKind>(kind))
                            == static_cast<AtlasKind>(sheet);

                    if (owned
                        && (kBuildingSprites[kind] == debris
                            || kBuildingSprites[kind] == fire))
                    {
                        return false;
                    }
                }
            }

            return true;
        }(),
        "a ruin sprite must have a slot of its own in every sheet");

    // In the 1x1 sheet the ruins also share with roads and walkers.
    // Past the sixteen junctions, and above the first walker row.
    static_assert(
        kDebrisSprites[0] > kRoadSpriteCount
            && kFireSprites[0] > kRoadSpriteCount
            && kDebrisSprites[0] < kFirstWalkerRow * kAtlasColumns
            && kFireSprites[0] < kFirstWalkerRow * kAtlasColumns,
        "the 1x1 ruin sprites must dodge the roads and the walkers");

    /**
     * @brief Get the sprite a cell with nothing on it is drawn from.
     * @return The ground sprite's rectangle, in the 1x1 sheet's pixels.
     */
    [[nodiscard]] constexpr Rect groundTile() noexcept
    {
        return spriteRect(AtlasKind::OneByOne, kGroundSprite);
    }

    /**
     * @brief Get the sprite a road with these links is drawn from.
     * @param links Which neighbours the road runs to, as linkBit() bits;
     * anything outside kLinkMask is ignored.
     * @return The road sprite's rectangle, in the 1x1 sheet's pixels.
     */
    [[nodiscard]] constexpr Rect roadTile(std::uint8_t links) noexcept
    {
        return spriteRect(
            AtlasKind::OneByOne, kRoadSpriteByLinks[links & kLinkMask]);
    }

    /**
     * @brief Get the sprite a walker facing this way is drawn from.
     *
     * Which frame to ask for is WalkerMotion's walkerFrame(); this
     * only turns the pair into pixels, so the sheet's layout stays in
     * this one header.
     *
     * @param facing The direction the walker is facing.
     * @param frame Which frame of the facing's walk cycle to show;
     * one past the last wraps round, as spriteRect()'s index does,
     * and the default is the standing frame.
     * @return The walker sprite's rectangle, in the 1x1 sheet's pixels.
     */
    [[nodiscard]] constexpr Rect walkerTile(
        Direction facing, std::uint32_t frame = 0) noexcept
    {
        const auto row = kFirstWalkerRow
            + static_cast<std::uint32_t>(
                directionIndex(facing) % kDirectionCount);

        return spriteRect(
            AtlasKind::OneByOne,
            row * kAtlasColumns + frame % kWalkCycleFrames);
    }

    /**
     * @brief Get the sprite a building of this kind is drawn from.
     * @param kind The building's kind.
     * @return The building sprite's rectangle, in the pixels of the
     * sheet buildingAtlasOf() names for it.
     */
    [[nodiscard]] constexpr Rect buildingTile(BuildingKind kind) noexcept
    {
        return spriteRect(
            buildingAtlasOf(kind),
            kBuildingSprites[buildingKindIndex(kind) % kBuildingKindCount]);
    }

    /**
     * @brief Get the sprite a ruin is drawn from.
     *
     * The sheet is the standing building's own, derived from the kind
     * the ruin remembers, so a burnt 2x2 farm smoulders across the
     * very diamond its block claims and its debris covers it after.
     *
     * @param state Whether it is still alight.
     * @param kind What stood there before the fire.
     * @return The sprite's rectangle, in the pixels of the sheet
     * buildingAtlasOf() names for the kind.
     */
    [[nodiscard]] constexpr Rect ruinTile(
        RuinState state, BuildingKind kind) noexcept
    {
        const auto sheet = buildingAtlasOf(kind);
        const auto slot = atlasKindIndex(sheet) % kAtlasKindCount;

        return spriteRect(
            sheet,
            state == RuinState::Burning ? kFireSprites[slot]
                                        : kDebrisSprites[slot]);
    }

    /**
     * @brief Get which sheet a tool's placement is drawn from.
     *
     * buildingAtlasOf() for a building tool, and the 1x1 sheet for the
     * road tool, which is the same decision toolTile() makes about the
     * rectangle -- the two answer one question in two halves, and both
     * branch on buildingKindOf() so they cannot branch apart.
     *
     * @param tool The selected tool.
     * @return The sheet the tool's ghost and placement blit from.
     */
    [[nodiscard]] constexpr AtlasKind toolAtlasOf(BuildTool tool) noexcept
    {
        const auto kind = buildingKindOf(tool);

        return kind.has_value() ? buildingAtlasOf(*kind)
                                : AtlasKind::OneByOne;
    }

    /**
     * @brief Get the sprite a tool's placement would be drawn from.
     *
     * The one place the palette's choice becomes art, so the ghost and
     * the thing it stands for cannot come from two different decisions.
     *
     * @param tool The selected tool.
     * @param links Which neighbours a road would run to; ignored for
     * every other tool.
     * @return The sprite's rectangle, in toolAtlasOf()'s sheet's pixels.
     */
    [[nodiscard]] constexpr Rect toolTile(
        BuildTool tool, std::uint8_t links) noexcept
    {
        const auto kind = buildingKindOf(tool);

        return kind.has_value() ? buildingTile(*kind) : roadTile(links);
    }

} // namespace antwika::game
