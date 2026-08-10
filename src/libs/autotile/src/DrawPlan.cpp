#include "antwika/autotile/DrawPlan.hpp"

#include <algorithm>
#include <array>
#include <cstddef>

#include "antwika/autotile/Cutaway.hpp"
#include "antwika/autotile/SheetLayout.hpp"

namespace antwika::autotile
{

    namespace
    {
        using geometry::GridCell;
        using tilemap::Overlay;
        using tilemap::TerrainClass;
        using tilemap::TileMap;

        constexpr std::array kDrawOrder = {
            TerrainClass::Water,
            TerrainClass::Floor,
            TerrainClass::Path,
            TerrainClass::Stair,
            TerrainClass::Cliff,
            TerrainClass::Wall,
        };

        constexpr std::uint8_t kFullMask = 15;
        constexpr std::uint8_t kWaterFrameB = 3;
        constexpr std::uint32_t kWaterPeriod = 30;
        constexpr std::uint8_t kShadeBelow = 192;
        constexpr std::uint8_t kDenseShadeBelow = 96;

        struct LevelRange final
        {
            std::int32_t lowest = 0;
            std::int32_t highest = 0;
        };

        [[nodiscard]] LevelRange levelsOf(const TileMap &map)
        {
            LevelRange range{};

            for (std::uint32_t row = 0; row < map.rows(); ++row)
            {
                for (std::uint32_t column = 0; column < map.columns();
                     ++column)
                {
                    const auto height =
                        map.at(GridCell{.column = column, .row = row})
                            .height;

                    range.lowest = std::min(range.lowest, height);
                    range.highest = std::max(range.highest, height);
                }
            }

            return range;
        }

        [[nodiscard]] std::size_t indexOf(
            const TileMap &map, const GridCell cell)
        {
            return static_cast<std::size_t>(cell.row) * map.columns()
                   + cell.column;
        }

        [[nodiscard]] std::uint64_t positionHash(
            const std::int64_t dualColumn,
            const std::int64_t dualRow) noexcept
        {
            constexpr std::uint64_t kColumnMix = 0x9E3779B97F4A7C15ULL;
            constexpr std::uint64_t kRowMix = 0xBF58476D1CE4E5B9ULL;
            constexpr std::uint64_t kFinalMix = 0x94D049BB133111EBULL;

            auto mixed =
                static_cast<std::uint64_t>(dualColumn) * kColumnMix
                ^ static_cast<std::uint64_t>(dualRow) * kRowMix;

            mixed ^= mixed >> 31;
            mixed *= kFinalMix;
            mixed ^= mixed >> 29;

            return mixed;
        }

        [[nodiscard]] std::uint8_t scatteredVariant(
            const std::uint64_t hash) noexcept
        {
            const auto bucket = hash % 4;

            if (bucket < 2)
            {
                return 0;
            }

            return static_cast<std::uint8_t>(bucket - 1);
        }

        [[nodiscard]] std::uint8_t waterVariant(
            const std::uint64_t hash,
            const std::uint32_t clock) noexcept
        {
            if ((clock / kWaterPeriod + hash) % 2 == 0)
            {
                return 0;
            }

            return kWaterFrameB;
        }

        [[nodiscard]] std::uint8_t surfaceVariant(
            const TerrainClass terrain,
            const std::int64_t dualColumn,
            const std::int64_t dualRow,
            const std::uint8_t mask,
            const std::uint32_t clock) noexcept
        {
            if (mask != kFullMask)
            {
                return 0;
            }

            const auto hash = positionHash(dualColumn, dualRow);

            if (terrain == TerrainClass::Water)
            {
                return waterVariant(hash, clock);
            }

            return scatteredVariant(hash);
        }

        class PlanBuilder final
        {
        public:
            PlanBuilder(
                const TileMap &map,
                const GridCell player,
                const std::int32_t playerHeight,
                const std::uint32_t clock)
                : map(map),
                  playerHeight(playerHeight),
                  clock(clock),
                  hidden(cutawayHidden(map, player, playerHeight))
            {
            }

            [[nodiscard]] DrawPlan build()
            {
                const auto range = levelsOf(map);

                for (auto level = range.lowest;
                     level <= range.highest;
                     ++level)
                {
                    addFaces(level);
                    addSurfaces(level);
                    addBridges(level);
                }

                addShades();

                return plan;
            }

        private:
            [[nodiscard]] bool contributes(
                const std::int64_t column,
                const std::int64_t row,
                const std::int32_t level,
                const TerrainClass terrain) const
            {
                if (column < 0 || row < 0
                    || column >= map.columns() || row >= map.rows())
                {
                    return false;
                }

                const auto cell = GridCell{
                    .column = static_cast<std::uint32_t>(column),
                    .row = static_cast<std::uint32_t>(row)};

                if (hidden[indexOf(map, cell)] && level > playerHeight)
                {
                    return false;
                }

                const auto &held = map.at(cell);

                return held.height == level && held.terrain == terrain;
            }

            [[nodiscard]] bool cutAway(
                const GridCell cell, const std::int32_t level) const
            {
                return hidden[indexOf(map, cell)]
                       && level > playerHeight;
            }

            void addSurfaces(const std::int32_t level)
            {
                for (const auto terrain : kDrawOrder)
                {
                    for (std::int64_t dualRow = 0;
                         dualRow <= map.rows();
                         ++dualRow)
                    {
                        for (std::int64_t dualColumn = 0;
                             dualColumn <= map.columns();
                             ++dualColumn)
                        {
                            addSurface(
                                dualColumn, dualRow, level, terrain);
                        }
                    }
                }
            }

            void addSurface(
                const std::int64_t dualColumn,
                const std::int64_t dualRow,
                const std::int32_t level,
                const TerrainClass terrain)
            {
                std::uint8_t mask = 0;

                if (contributes(
                        dualColumn - 1, dualRow - 1, level, terrain))
                {
                    mask |= 1;
                }

                if (contributes(dualColumn, dualRow - 1, level, terrain))
                {
                    mask |= 2;
                }

                if (contributes(dualColumn - 1, dualRow, level, terrain))
                {
                    mask |= 4;
                }

                if (contributes(dualColumn, dualRow, level, terrain))
                {
                    mask |= 8;
                }

                if (mask == 0)
                {
                    return;
                }

                plan.push_back(TileDraw{
                    .terrain = terrain,
                    .piece = TilePiece::Surface,
                    .mask = mask,
                    .variant = surfaceVariant(
                        terrain, dualColumn, dualRow, mask, clock),
                    .screen = {
                        .x = static_cast<std::int32_t>(dualColumn)
                             * kUnit
                             - kHalfTile,
                        .y = static_cast<std::int32_t>(dualRow) * kUnit
                             - kHalfTile
                             - level * kLevelRise}});
            }

            void addFaces(const std::int32_t level)
            {
                for (std::uint32_t row = 0; row < map.rows(); ++row)
                {
                    for (std::uint32_t column = 0;
                         column < map.columns();
                         ++column)
                    {
                        addFace(
                            GridCell{.column = column, .row = row},
                            level);
                    }
                }
            }

            void addFace(const GridCell cell, const std::int32_t level)
            {
                const auto height = map.at(cell).height;

                if (height != level)
                {
                    return;
                }

                const auto south = GridCell{
                    .column = cell.column, .row = cell.row + 1};

                const auto below = south.row < map.rows()
                                       ? map.at(south).height
                                       : 0;

                auto top = height;

                if (hidden[indexOf(map, cell)])
                {
                    top = std::min(top, playerHeight);
                }

                for (auto band = below + 1; band <= top; ++band)
                {
                    const auto piece = band == height
                                           ? TilePiece::WallRim
                                           : TilePiece::WallBand;

                    const auto y =
                        static_cast<std::int32_t>(south.row) * kUnit
                        - band * kLevelRise;

                    const auto x =
                        static_cast<std::int32_t>(cell.column) * kUnit;

                    plan.push_back(TileDraw{
                        .terrain = TerrainClass::Cliff,
                        .piece = piece,
                        .mask = 0,
                        .screen = {.x = x, .y = y}});
                    plan.push_back(TileDraw{
                        .terrain = TerrainClass::Cliff,
                        .piece = piece,
                        .mask = 0,
                        .screen = {.x = x + kHalfTile, .y = y}});
                }
            }

            void addBridges(const std::int32_t level)
            {
                for (std::uint32_t row = 0; row < map.rows(); ++row)
                {
                    for (std::uint32_t column = 0;
                         column < map.columns();
                         ++column)
                    {
                        addBridge(
                            GridCell{.column = column, .row = row},
                            level);
                    }
                }
            }

            void addBridge(
                const GridCell cell, const std::int32_t level)
            {
                const auto &held = map.at(cell);

                if (held.height != level
                    || held.overlay != Overlay::Bridge)
                {
                    return;
                }

                if (cutAway(cell, level))
                {
                    return;
                }

                pushQuad(
                    TilePiece::BridgeDeck, held.terrain, cell, level);
            }

            void addShades()
            {
                for (std::uint32_t row = 0; row < map.rows(); ++row)
                {
                    for (std::uint32_t column = 0;
                         column < map.columns();
                         ++column)
                    {
                        addShade(
                            GridCell{.column = column, .row = row});
                    }
                }
            }

            void addShade(const GridCell cell)
            {
                const auto &held = map.at(cell);

                if (held.light >= kShadeBelow)
                {
                    return;
                }

                if (cutAway(cell, held.height))
                {
                    return;
                }

                pushQuad(
                    TilePiece::Shade, held.terrain, cell, held.height);

                if (held.light < kDenseShadeBelow)
                {
                    pushQuad(
                        TilePiece::Shade,
                        held.terrain,
                        cell,
                        held.height);
                }
            }

            void pushQuad(
                const TilePiece piece,
                const TerrainClass terrain,
                const GridCell cell,
                const std::int32_t level)
            {
                const auto left =
                    static_cast<std::int32_t>(cell.column) * kUnit;

                const auto top =
                    static_cast<std::int32_t>(cell.row) * kUnit
                    - level * kLevelRise;

                for (std::int32_t part = 0; part < 4; ++part)
                {
                    plan.push_back(TileDraw{
                        .terrain = terrain,
                        .piece = piece,
                        .mask = 0,
                        .screen = {
                            .x = left + part % 2 * kHalfTile,
                            .y = top + part / 2 * kHalfTile}});
                }
            }

            const TileMap &map;
            std::int32_t playerHeight;
            std::uint32_t clock;
            std::vector<bool> hidden;
            DrawPlan plan{};
        };
    }

    DrawPlan buildDrawPlan(
        const TileMap &map,
        const GridCell player,
        const std::int32_t playerHeight,
        const std::uint32_t clock)
    {
        return PlanBuilder(map, player, playerHeight, clock).build();
    }

}
