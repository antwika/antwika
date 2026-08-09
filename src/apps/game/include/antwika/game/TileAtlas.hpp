#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <antwika/enums/Enumeration.hpp>
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

    inline constexpr Size kIsoTileSize{.width = 32, .height = 16};

    enum class AtlasKind : std::uint8_t
    {
        OneByOne = 0,
        TwoByTwo,
        ThreeByThree,
    };

    [[nodiscard]] constexpr AtlasKind enumBound(AtlasKind) noexcept
    {
        return AtlasKind::ThreeByThree;
    }

    inline constexpr std::size_t kAtlasKindCount =
        antwika::enums::kCount<AtlasKind>;

    [[nodiscard]] constexpr std::size_t atlasKindIndex(
        const AtlasKind kind) noexcept
    {
        return antwika::enums::index(kind);
    }

    struct AtlasSpec final
    {
        Size spriteSize{};

        Point pivot{};

        Size isometric{};

        std::uint32_t columns = 0;

        std::uint32_t rows = 0;

        [[nodiscard]] constexpr std::uint32_t slots() const noexcept
        {
            return columns * rows;
        }

        [[nodiscard]] constexpr Size sheetSize() const noexcept
        {
            return Size{
                .width = columns * spriteSize.width,
                .height = rows * spriteSize.height};
        }

        [[nodiscard]] bool operator==(const AtlasSpec &other) const =
            default;
    };

    struct AtlasSpecs final
    {
        std::array<AtlasSpec, kAtlasKindCount> byKind{};

        AtlasSpec walker{};

        [[nodiscard]] constexpr const AtlasSpec &of(
            AtlasKind kind) const noexcept
        {
            return antwika::enums::pick(byKind, kind);
        }

        [[nodiscard]] bool operator==(const AtlasSpecs &other) const =
            default;
    };

    [[nodiscard]] constexpr AtlasSpec atlasSpec(
        const AtlasSpecs &specs, AtlasKind kind) noexcept
    {
        return specs.of(kind);
    }

    [[nodiscard]] constexpr Rect spriteRect(
        const AtlasSpec &spec, std::uint32_t index) noexcept
    {
        if (spec.slots() == 0)
        {
            return Rect{};
        }

        const auto wrapped = index % spec.slots();

        return Rect{
            .origin =
                {.x = static_cast<std::int32_t>(
                     (wrapped % spec.columns) * spec.spriteSize.width),
                 .y = static_cast<std::int32_t>(
                     (wrapped / spec.columns) * spec.spriteSize.height)},
            .size = spec.spriteSize};
    }

    [[nodiscard]] constexpr Rect spriteRect(
        const AtlasSpecs &specs,
        AtlasKind kind,
        std::uint32_t index) noexcept
    {
        return spriteRect(specs.of(kind), index);
    }

    inline constexpr std::uint32_t kGroundSprite = 0;

    [[nodiscard]] constexpr std::uint8_t linkBit(
        Direction direction) noexcept
    {
        return static_cast<std::uint8_t>(
            1U << (directionIndex(direction) % kDirectionCount));
    }

    inline constexpr std::uint8_t kLinkMask =
        linkBit(Direction::North) | linkBit(Direction::East)
        | linkBit(Direction::South) | linkBit(Direction::West);

    inline constexpr std::uint32_t kRoadSpriteCount = 16;

    inline constexpr std::array<std::uint8_t, kRoadSpriteCount>
        kRoadSpriteByLinks{
            1,
            2,
            3,
            6,
            4,
            15,
            7,
            10,
            5,
            9,
            14,
            13,
            8,
            12,
            11,
            16,
        };

    inline constexpr std::uint32_t kWalkCycleFrames = 4;

    inline constexpr std::array<std::uint8_t, kDirectionCount>
        kWalkerRowByFacing{3, 0, 1, 2};

    inline constexpr std::uint32_t kWalkerSheetRows =
        static_cast<std::uint32_t>(kDirectionCount);

    inline constexpr std::array<std::uint8_t, kBuildingKindCount>
        kBuildingSprites{
            17,
            0,
            1,
            2,
            0,
            3,
            18,
            19,
            20,
            21,
        };

    inline constexpr std::array<std::uint8_t, kAtlasKindCount>
        kDebrisSprites{22, 8, 8};

    inline constexpr std::array<std::uint8_t, kAtlasKindCount>
        kFireSprites{23, 9, 9};

    [[nodiscard]] constexpr AtlasKind buildingAtlasOf(
        BuildingKind kind) noexcept
    {
        return static_cast<AtlasKind>(footprintOf(kind).width - 1);
    }

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

    static_assert(
        kLinkMask == (1U << kDirectionCount) - 1U,
        "kDirectionCount must count exactly the named directions");

    static_assert(
        []
        {
            std::array<bool, kDirectionCount> seen{};

            for (const auto row : kWalkerRowByFacing)
            {
                if (static_cast<std::uint32_t>(row) >= kWalkerSheetRows
                    || seen[row])
                {
                    return false;
                }

                seen[row] = true;
            }

            return true;
        }(),
        "the walker sheet must give every facing a row of its own");

    static_assert(
        kWalkerRowByFacing[directionIndex(Direction::East)] == 0,
        "the walker sheet must open with the facing that walks down "
        "and to the right");

    static_assert(
        []
        {
            for (std::size_t index = 0; index < kDirectionCount; ++index)
            {
                const auto facing = static_cast<Direction>(index);
                const auto row = kWalkerRowByFacing[index];
                const auto next =
                    kWalkerRowByFacing[directionIndex(turnRight(facing))];

                if (static_cast<std::uint32_t>(next)
                    != (static_cast<std::uint32_t>(row) + 1U)
                        % kWalkerSheetRows)
                {
                    return false;
                }
            }

            return true;
        }(),
        "the walker sheet must turn a quarter to the right with every "
        "row it descends");

    static_assert(
        kRoadSpriteCount == 1U << kDirectionCount,
        "there must be a road sprite for every link mask");

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

    static_assert(
        []
        {
            for (std::size_t sheet = 0; sheet < kAtlasKindCount; ++sheet)
            {
                const auto debris = kDebrisSprites[sheet];
                const auto fire = kFireSprites[sheet];

                if (debris == fire)
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

    [[nodiscard]] constexpr Rect groundTile(
        const AtlasSpecs &specs) noexcept
    {
        return spriteRect(specs, AtlasKind::OneByOne, kGroundSprite);
    }

    [[nodiscard]] constexpr Rect roadTile(
        const AtlasSpecs &specs, std::uint8_t links) noexcept
    {
        return spriteRect(
            specs,
            AtlasKind::OneByOne,
            kRoadSpriteByLinks[links & kLinkMask]);
    }

    [[nodiscard]] constexpr Rect walkerTile(
        const AtlasSpecs &specs,
        Direction facing,
        std::uint32_t frame = 0) noexcept
    {
        const auto row = static_cast<std::uint32_t>(
            kWalkerRowByFacing[directionIndex(facing) % kDirectionCount]);

        return spriteRect(
            specs.walker,
            row * specs.walker.columns + frame % kWalkCycleFrames);
    }

    [[nodiscard]] constexpr Rect buildingTile(
        const AtlasSpecs &specs, BuildingKind kind) noexcept
    {
        return spriteRect(
            specs,
            buildingAtlasOf(kind),
            kBuildingSprites[buildingKindIndex(kind) % kBuildingKindCount]);
    }

    [[nodiscard]] constexpr Rect ruinTile(
        const AtlasSpecs &specs,
        RuinState state,
        BuildingKind kind) noexcept
    {
        const auto sheet = buildingAtlasOf(kind);
        const auto slot = atlasKindIndex(sheet) % kAtlasKindCount;

        return spriteRect(
            specs,
            sheet,
            state == RuinState::Burning ? kFireSprites[slot]
                                        : kDebrisSprites[slot]);
    }

    [[nodiscard]] constexpr AtlasKind toolAtlasOf(BuildTool tool) noexcept
    {
        const auto kind = buildingKindOf(tool);

        return kind.has_value() ? buildingAtlasOf(*kind)
                                : AtlasKind::OneByOne;
    }

    [[nodiscard]] constexpr Rect toolTile(
        const AtlasSpecs &specs,
        BuildTool tool,
        std::uint8_t links) noexcept
    {
        const auto kind = buildingKindOf(tool);

        return kind.has_value() ? buildingTile(specs, *kind)
                                : roadTile(specs, links);
    }

}
