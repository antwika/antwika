#include "antwika/autotile/DrawPlan.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <vector>

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
            const auto bucket = hash % (2 * kVariantSlots);

            if (bucket < kVariantSlots)
            {
                return 0;
            }

            return static_cast<std::uint8_t>(
                bucket - kVariantSlots + 1);
        }

        [[nodiscard]] bool edgeOn(
            const SheetConnectors &sheet,
            const std::int32_t variant,
            const std::uint8_t edge) noexcept
        {
            return (sheet.edges[static_cast<std::size_t>(variant)]
                    & edge)
                   != 0;
        }

        [[nodiscard]] bool fitsNeighbours(
            const SheetConnectors &sheet,
            const std::int32_t variant,
            const std::int32_t west,
            const std::int32_t north,
            const bool useWest,
            const bool useNorth) noexcept
        {
            if (useWest && west >= 0
                && edgeOn(sheet, variant, kEdgeWest)
                       != edgeOn(sheet, west, kEdgeEast))
            {
                return false;
            }

            if (useNorth && north >= 0
                && edgeOn(sheet, variant, kEdgeNorth)
                       != edgeOn(sheet, north, kEdgeSouth))
            {
                return false;
            }

            return true;
        }

        [[nodiscard]] bool quadrantEdgeOn(
            const SheetConnectors &sheet,
            const std::int32_t slot,
            const std::uint8_t edge) noexcept
        {
            return (sheet.quadrants[static_cast<std::size_t>(slot)]
                    & edge)
                   != 0;
        }

        [[nodiscard]] bool quadrantFits(
            const SheetConnectors &sheet,
            const std::int32_t slot,
            const std::int32_t west,
            const std::int32_t north,
            const bool useWest,
            const bool useNorth) noexcept
        {
            if (useWest && west >= 0
                && quadrantEdgeOn(sheet, slot, kEdgeWest)
                       != quadrantEdgeOn(sheet, west, kEdgeEast))
            {
                return false;
            }

            if (useNorth && north >= 0
                && quadrantEdgeOn(sheet, slot, kEdgeNorth)
                       != quadrantEdgeOn(sheet, north, kEdgeSouth))
            {
                return false;
            }

            return true;
        }

        [[nodiscard]] std::uint8_t chooseQuadrant(
            const SheetConnectors &sheet,
            const std::uint64_t hash,
            const std::int32_t west,
            const std::int32_t north) noexcept
        {
            std::array<std::uint8_t, kQuadrantSlots> declared{};
            std::size_t count = 0;

            for (std::size_t slot = 0; slot < kQuadrantSlots;
                 ++slot)
            {
                if ((sheet.quadrantMask & (1U << slot)) != 0)
                {
                    declared[count] =
                        static_cast<std::uint8_t>(slot);
                    ++count;
                }
            }

            const auto base = declared[0];
            const auto bucket = hash % (2 * count);
            const auto preliminary =
                bucket < count
                    ? base
                    : declared[bucket - count];

            if (quadrantFits(
                    sheet, preliminary, west, north, true, true))
            {
                return preliminary;
            }

            constexpr std::array<std::array<bool, 2>, 3> kPasses{
                {{true, true}, {true, false}, {false, true}}};

            for (const auto &pass : kPasses)
            {
                std::array<std::uint8_t, kQuadrantSlots>
                    candidates{};
                std::size_t fitting = 0;
                bool baseFits = false;

                for (std::size_t at = 0; at < count; ++at)
                {
                    if (!quadrantFits(
                            sheet,
                            declared[at],
                            west,
                            north,
                            pass[0],
                            pass[1]))
                    {
                        continue;
                    }

                    baseFits = baseFits || declared[at] == base;
                    candidates[fitting] = declared[at];
                    ++fitting;
                }

                if (fitting == 0)
                {
                    continue;
                }

                return baseFits ? base
                                : candidates[hash % fitting];
            }

            return base;
        }

        [[nodiscard]] std::uint8_t chooseVariant(
            const SheetConnectors &sheet,
            const std::uint64_t hash,
            const std::int32_t west,
            const std::int32_t north) noexcept
        {
            const auto preliminary = scatteredVariant(hash);

            if (fitsNeighbours(
                    sheet, preliminary, west, north, true, true))
            {
                return preliminary;
            }

            constexpr std::array<std::array<bool, 2>, 3> kPasses{
                {{true, true}, {true, false}, {false, true}}};

            for (const auto &pass : kPasses)
            {
                std::array<std::uint8_t, 8> candidates{};
                std::size_t count = 0;
                bool baseFits = false;

                for (std::int32_t variant = 0; variant < 8;
                     ++variant)
                {
                    if (!fitsNeighbours(
                            sheet,
                            variant,
                            west,
                            north,
                            pass[0],
                            pass[1]))
                    {
                        continue;
                    }

                    baseFits = baseFits || variant == 0;
                    candidates[count] =
                        static_cast<std::uint8_t>(variant);
                    ++count;
                }

                if (count == 0)
                {
                    continue;
                }

                return baseFits ? 0 : candidates[hash % count];
            }

            return 0;
        }

        class PlanBuilder final
        {
        public:
            PlanBuilder(
                const TileMap &map,
                const GridCell player,
                const std::int32_t playerHeight,
                const std::uint32_t clock,
                const TerrainConnectors &connectors)
                : map(map),
                  playerHeight(playerHeight),
                  clock(clock),
                  connectors(connectors),
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
                const auto width =
                    static_cast<std::size_t>(map.columns()) + 1;

                for (const auto terrain : kDrawOrder)
                {
                    if (connectors[enums::index(terrain)]
                            .quadrantMask
                        != 0)
                    {
                        addQuadrantSurfaces(level, terrain, width);
                        continue;
                    }

                    std::vector<std::int32_t> previousRow(
                        width, -1);
                    std::vector<std::int32_t> currentRow(width, -1);

                    for (std::int64_t dualRow = 0;
                         dualRow <= map.rows();
                         ++dualRow)
                    {
                        std::ranges::fill(currentRow, -1);

                        for (std::int64_t dualColumn = 0;
                             dualColumn <= map.columns();
                             ++dualColumn)
                        {
                            const auto at = static_cast<
                                std::size_t>(dualColumn);
                            const auto west =
                                dualColumn > 0
                                    ? currentRow[at - 1]
                                    : -1;
                            const auto north = previousRow[at];

                            currentRow[at] = addSurface(
                                dualColumn,
                                dualRow,
                                level,
                                terrain,
                                west,
                                north);
                        }

                        std::swap(previousRow, currentRow);
                    }
                }
            }

            /**
             * @brief Assembles a quadrant terrain's surfaces.
             *
             * Ensures: interior full-mask tiles become four 8x8
             *          quadrant draws on the uniform lattice while
             *          every partial mask keeps its normal surface
             *          piece.
             */
            void addQuadrantSurfaces(
                const std::int32_t level,
                const TerrainClass terrain,
                const std::size_t width)
            {
                const auto rows =
                    static_cast<std::size_t>(map.rows()) + 1;
                std::vector<std::uint8_t> interior(
                    width * rows, 0);

                for (std::int64_t dualRow = 0;
                     dualRow <= map.rows();
                     ++dualRow)
                {
                    for (std::int64_t dualColumn = 0;
                         dualColumn <= map.columns();
                         ++dualColumn)
                    {
                        const auto mask = surfaceMask(
                            dualColumn, dualRow, level, terrain);

                        if (mask == 0)
                        {
                            continue;
                        }

                        if (mask == kFullMask)
                        {
                            interior
                                [static_cast<std::size_t>(dualRow)
                                     * width
                                 + static_cast<std::size_t>(
                                     dualColumn)] = 1;
                            continue;
                        }

                        plan.push_back(TileDraw{
                            .terrain = terrain,
                            .piece = TilePiece::Surface,
                            .mask = mask,
                            .variant = 0,
                            .screen = {
                                .x = static_cast<std::int32_t>(
                                         dualColumn)
                                         * kUnit
                                     - kHalfTile,
                                .y = static_cast<std::int32_t>(
                                         dualRow)
                                         * kUnit
                                     - kHalfTile
                                     - level * kLevelRise}});
                    }
                }

                addQuadrantLattice(level, terrain, interior, width);
            }

            void addQuadrantLattice(
                const std::int32_t level,
                const TerrainClass terrain,
                const std::vector<std::uint8_t> &interior,
                const std::size_t width)
            {
                const auto &sheet =
                    connectors[enums::index(terrain)];
                const auto lattice = width * 2;
                std::vector<std::int32_t> previousRow(lattice, -1);
                std::vector<std::int32_t> currentRow(lattice, -1);

                for (std::size_t qr = 0;
                     qr < (static_cast<std::size_t>(map.rows()) + 1)
                              * 2;
                     ++qr)
                {
                    std::ranges::fill(currentRow, -1);

                    for (std::size_t qc = 0; qc < lattice; ++qc)
                    {
                        if (interior[(qr / 2) * width + qc / 2]
                            == 0)
                        {
                            continue;
                        }

                        const auto west =
                            qc > 0 ? currentRow[qc - 1] : -1;
                        const auto north = previousRow[qc];
                        const auto slot = chooseQuadrant(
                            sheet,
                            positionHash(
                                static_cast<std::int64_t>(qc),
                                static_cast<std::int64_t>(qr)),
                            west,
                            north);

                        currentRow[qc] = slot;
                        plan.push_back(TileDraw{
                            .terrain = terrain,
                            .piece = TilePiece::Quadrant,
                            .mask = 0,
                            .variant = slot,
                            .screen = {
                                .x = static_cast<std::int32_t>(qc)
                                         * kHalfTile
                                     - kHalfTile,
                                .y = static_cast<std::int32_t>(qr)
                                         * kHalfTile
                                     - kHalfTile
                                     - level * kLevelRise}});
                    }

                    std::swap(previousRow, currentRow);
                }
            }

            [[nodiscard]] std::uint8_t surfaceMask(
                const std::int64_t dualColumn,
                const std::int64_t dualRow,
                const std::int32_t level,
                const TerrainClass terrain) const
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

                return mask;
            }

            [[nodiscard]] std::int32_t addSurface(
                const std::int64_t dualColumn,
                const std::int64_t dualRow,
                const std::int32_t level,
                const TerrainClass terrain,
                const std::int32_t west,
                const std::int32_t north)
            {
                const auto mask = surfaceMask(
                    dualColumn, dualRow, level, terrain);

                if (mask == 0)
                {
                    return -1;
                }

                std::int32_t chosen = -1;
                std::uint8_t variant = 0;

                if (mask == kFullMask)
                {
                    const auto hash =
                        positionHash(dualColumn, dualRow);
                    const auto &sheet =
                        connectors[enums::index(terrain)];

                    chosen = chooseVariant(sheet, hash, west, north);
                    variant = static_cast<std::uint8_t>(chosen);

                    if (terrain == TerrainClass::Water
                        && chosen == 0)
                    {
                        variant =
                            (clock / kWaterPeriod + hash) % 2 == 0
                                ? 0
                                : kWaterFrameBVariant;
                    }
                }

                plan.push_back(TileDraw{
                    .terrain = terrain,
                    .piece = TilePiece::Surface,
                    .mask = mask,
                    .variant = variant,
                    .screen = {
                        .x = static_cast<std::int32_t>(dualColumn)
                             * kUnit
                             - kHalfTile,
                        .y = static_cast<std::int32_t>(dualRow) * kUnit
                             - kHalfTile
                             - level * kLevelRise}});

                return chosen;
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
            const TerrainConnectors &connectors;
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
        const TerrainConnectors allConnected{};

        return PlanBuilder(
                   map, player, playerHeight, clock, allConnected)
            .build();
    }

    DrawPlan buildDrawPlan(
        const TileMap &map,
        const GridCell player,
        const std::int32_t playerHeight,
        const std::uint32_t clock,
        const TerrainConnectors &connectors)
    {
        return PlanBuilder(
                   map, player, playerHeight, clock, connectors)
            .build();
    }

}
