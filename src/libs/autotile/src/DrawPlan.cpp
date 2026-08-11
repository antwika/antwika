#include "antwika/autotile/DrawPlan.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <antwika/tileset/Atlas.hpp>
#include <antwika/tileset/Sprite.hpp>

#include "antwika/autotile/Cutaway.hpp"
#include "antwika/autotile/Metrics.hpp"

namespace antwika::autotile
{

    namespace
    {
        using geometry::GridCell;
        using tilemap::Overlay;
        using tilemap::TerrainClass;
        using tilemap::TileMap;
        using tileset::Side;
        using tileset::Sprite;

        constexpr std::array kDrawOrder = {
            TerrainClass::Water,
            TerrainClass::Floor,
            TerrainClass::Path,
            TerrainClass::Stair,
            TerrainClass::Cliff,
            TerrainClass::Wall,
        };

        constexpr std::uint32_t kFramePeriod = 30;
        constexpr std::uint8_t kShadeBelow = 192;
        constexpr std::uint8_t kDenseShadeBelow = 96;

        constexpr auto kNorth = enums::index(Side::North);
        constexpr auto kEast = enums::index(Side::East);
        constexpr auto kSouth = enums::index(Side::South);
        constexpr auto kWest = enums::index(Side::West);

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
                    const auto cell =
                        GridCell{.column = column, .row = row};

                    for (const auto &slab : map.at(cell).slabs())
                    {
                        range.lowest =
                            std::min(range.lowest, slab.level);
                        range.highest =
                            std::max(range.highest, slab.level);
                    }
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

        [[nodiscard]] std::uint8_t frameOf(
            const Sprite &sprite, const std::uint32_t clock) noexcept
        {
            if (sprite.frameCount <= 1)
            {
                return 0;
            }

            return static_cast<std::uint8_t>(
                (clock / kFramePeriod) % sprite.frameCount);
        }

        [[nodiscard]] bool shapeFits(
            const Sprite &sprite,
            const std::array<bool, 4> &out) noexcept
        {
            for (std::size_t side = 0; side < 4; ++side)
            {
                if (out[side]
                    != (sprite.sockets[side] == tileset::kEdgeSocket))
                {
                    return false;
                }
            }

            return true;
        }

        [[nodiscard]] bool pairFits(
            const std::vector<Sprite> &sprites,
            const std::size_t candidate,
            const std::int32_t west,
            const std::int32_t north,
            const bool useWest,
            const bool useNorth) noexcept
        {
            const auto &sprite = sprites[candidate];

            if (useWest && west >= 0
                && sprites[static_cast<std::size_t>(west)]
                           .sockets[kEast]
                       != sprite.sockets[kWest])
            {
                return false;
            }

            if (useNorth && north >= 0
                && sprites[static_cast<std::size_t>(north)]
                           .sockets[kSouth]
                       != sprite.sockets[kNorth])
            {
                return false;
            }

            return true;
        }

        [[nodiscard]] std::size_t weightedPick(
            const std::vector<Sprite> &sprites,
            const std::vector<std::size_t> &candidates,
            const std::uint64_t hash)
        {
            std::uint64_t total = 0;

            for (const auto candidate : candidates)
            {
                total += sprites[candidate].weight;
            }

            auto roll = hash % total;

            for (std::size_t at = 0; at + 1 < candidates.size(); ++at)
            {
                const auto weight = sprites[candidates[at]].weight;

                if (roll < weight)
                {
                    return candidates[at];
                }

                roll -= weight;
            }

            return candidates.back();
        }

        [[nodiscard]] std::size_t chooseFrom(
            const std::vector<Sprite> &sprites,
            const std::vector<std::size_t> &declared,
            const std::uint64_t hash,
            const std::int32_t west,
            const std::int32_t north)
        {
            const auto preliminary =
                weightedPick(sprites, declared, hash);

            if (pairFits(
                    sprites, preliminary, west, north, true, true))
            {
                return preliminary;
            }

            constexpr std::array<std::array<bool, 2>, 3> kPasses{
                {{true, true}, {true, false}, {false, true}}};

            for (const auto &pass : kPasses)
            {
                std::vector<std::size_t> candidates{};

                for (const auto candidate : declared)
                {
                    if (pairFits(
                            sprites,
                            candidate,
                            west,
                            north,
                            pass[0],
                            pass[1]))
                    {
                        candidates.push_back(candidate);
                    }
                }

                if (!candidates.empty())
                {
                    return weightedPick(sprites, candidates, hash);
                }
            }

            return preliminary;
        }

        [[nodiscard]] std::size_t chooseSprite(
            const std::vector<Sprite> &sprites,
            const std::array<bool, 4> &out,
            const std::uint64_t hash,
            const std::int32_t west,
            const std::int32_t north)
        {
            std::vector<std::size_t> declared{};

            for (std::size_t at = 0; at < sprites.size(); ++at)
            {
                if (shapeFits(sprites[at], out))
                {
                    declared.push_back(at);
                }
            }

            if (!declared.empty())
            {
                return chooseFrom(
                    sprites, declared, hash, west, north);
            }

            std::vector<std::size_t> all{};

            for (std::size_t at = 0; at < sprites.size(); ++at)
            {
                all.push_back(at);
            }

            return chooseFrom(sprites, all, hash, west, north);
        }

        class PlanBuilder final
        {
        public:
            PlanBuilder(
                const TileMap &map,
                const GridCell player,
                const std::int32_t playerHeight,
                const std::uint32_t clock,
                const TilesetBindings &bindings)
                : map(map),
                  playerHeight(playerHeight),
                  clock(clock),
                  bindings(bindings),
                  hidden(cutawayHidden(map, player, playerHeight))
            {
                for (std::size_t at = 0; at < atlas.size(); ++at)
                {
                    atlas[at] =
                        tileset::atlasIndexOf(*bindings.byTerrain[at]);
                }
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
            /**
             * @brief Tells whether a cell surfaces the terrain.
             *
             * Requires: column and row lie inside the grid, which
             *           inside() settles before it calls this.
             */
            [[nodiscard]] bool contributes(
                const std::int64_t column,
                const std::int64_t row,
                const std::int32_t level,
                const TerrainClass terrain) const
            {
                const auto cell = GridCell{
                    .column = static_cast<std::uint32_t>(column),
                    .row = static_cast<std::uint32_t>(row)};

                if (hidden[indexOf(map, cell)] && level > playerHeight)
                {
                    return false;
                }

                const auto &held = map.at(cell);
                const auto *slab = held.slabAt(level);

                return slab != nullptr && slab->terrain == terrain
                       && held.slabAt(level + 1) == nullptr;
            }

            [[nodiscard]] bool cutAway(
                const GridCell cell, const std::int32_t level) const
            {
                return hidden[indexOf(map, cell)]
                       && level > playerHeight;
            }

            [[nodiscard]] bool inside(
                const std::int64_t latticeColumn,
                const std::int64_t latticeRow,
                const std::int32_t level,
                const TerrainClass terrain) const
            {
                return latticeColumn >= 0 && latticeRow >= 0
                       && latticeColumn < 2 * map.columns()
                       && latticeRow < 2 * map.rows()
                       && contributes(
                           latticeColumn / 2,
                           latticeRow / 2,
                           level,
                           terrain);
            }

            [[nodiscard]] std::array<bool, 4> bordersOf(
                const std::int64_t latticeColumn,
                const std::int64_t latticeRow,
                const std::int32_t level,
                const TerrainClass terrain) const
            {
                return {
                    !inside(
                        latticeColumn,
                        latticeRow - 1,
                        level,
                        terrain),
                    !inside(
                        latticeColumn + 1,
                        latticeRow,
                        level,
                        terrain),
                    !inside(
                        latticeColumn,
                        latticeRow + 1,
                        level,
                        terrain),
                    !inside(
                        latticeColumn - 1,
                        latticeRow,
                        level,
                        terrain)};
            }

            [[nodiscard]] geometry::Point latticeScreen(
                const std::int64_t latticeColumn,
                const std::int64_t latticeRow,
                const std::int32_t level) const
            {
                return {
                    .x = static_cast<std::int32_t>(latticeColumn)
                         * kHalfTile,
                    .y = static_cast<std::int32_t>(latticeRow)
                             * kHalfTile
                         - level * kLevelRise};
            }

            void addSurfaces(const std::int32_t level)
            {
                for (const auto terrain : kDrawOrder)
                {
                    assembleTerrain(level, terrain);
                }
            }

            void assembleTerrain(
                const std::int32_t level, const TerrainClass terrain)
            {
                const auto &set =
                    *bindings.byTerrain[enums::index(terrain)];
                const auto &base = set.layers[0].sprites;

                if (base.empty())
                {
                    return;
                }

                const auto columns =
                    static_cast<std::int64_t>(map.columns()) * 2;
                const auto rows =
                    static_cast<std::int64_t>(map.rows()) * 2;
                const auto &offsets =
                    atlas[enums::index(terrain)].layerRowOffsets;

                std::vector<std::int32_t> chosen(
                    static_cast<std::size_t>(columns * rows), -1);

                for (std::int64_t latticeRow = 0; latticeRow < rows;
                     ++latticeRow)
                {
                    for (std::int64_t latticeColumn = 0;
                         latticeColumn < columns;
                         ++latticeColumn)
                    {
                        if (!inside(
                                latticeColumn,
                                latticeRow,
                                level,
                                terrain))
                        {
                            continue;
                        }

                        const auto at = static_cast<std::size_t>(
                            latticeRow * columns + latticeColumn);
                        const auto west =
                            latticeColumn > 0 ? chosen[at - 1] : -1;
                        const auto north =
                            latticeRow > 0
                                ? chosen
                                      [at
                                       - static_cast<std::size_t>(
                                           columns)]
                                : -1;
                        const auto pick = chooseSprite(
                            base,
                            bordersOf(
                                latticeColumn,
                                latticeRow,
                                level,
                                terrain),
                            positionHash(latticeColumn, latticeRow),
                            west,
                            north);

                        chosen[at] = static_cast<std::int32_t>(pick);
                        plan.push_back(TileDraw{
                            .terrain = terrain,
                            .kind = DrawKind::Sprite,
                            .atlasRow = static_cast<std::uint16_t>(
                                offsets[0] + pick),
                            .frame = frameOf(base[pick], clock),
                            .screen = latticeScreen(
                                latticeColumn, latticeRow, level)});
                    }
                }

                for (std::size_t layer = 1; layer < set.layers.size();
                     ++layer)
                {
                    addDecor(level, terrain, layer, chosen);
                }
            }

            void addDecor(
                const std::int32_t level,
                const TerrainClass terrain,
                const std::size_t layerAt,
                const std::vector<std::int32_t> &chosen)
            {
                const auto &set =
                    *bindings.byTerrain[enums::index(terrain)];
                const auto &layer = set.layers[layerAt];

                if (layer.sprites.empty())
                {
                    return;
                }

                const auto columns =
                    static_cast<std::int64_t>(map.columns()) * 2;
                const auto rows =
                    static_cast<std::int64_t>(map.rows()) * 2;
                const auto &offsets =
                    atlas[enums::index(terrain)].layerRowOffsets;

                std::vector<std::int32_t> decor(chosen.size(), -1);

                for (std::int64_t latticeRow = 0; latticeRow < rows;
                     ++latticeRow)
                {
                    for (std::int64_t latticeColumn = 0;
                         latticeColumn < columns;
                         ++latticeColumn)
                    {
                        const auto at = static_cast<std::size_t>(
                            latticeRow * columns + latticeColumn);

                        if (chosen[at] < 0)
                        {
                            continue;
                        }

                        placeDecor(
                            set,
                            layer,
                            layerAt,
                            offsets,
                            chosen,
                            decor,
                            at,
                            latticeColumn,
                            latticeRow,
                            columns,
                            level,
                            terrain);
                    }
                }
            }

            void placeDecor(
                const tileset::Tileset &set,
                const tileset::Layer &layer,
                const std::size_t layerAt,
                const std::vector<std::uint32_t> &offsets,
                const std::vector<std::int32_t> &chosen,
                std::vector<std::int32_t> &decor,
                const std::size_t at,
                const std::int64_t latticeColumn,
                const std::int64_t latticeRow,
                const std::int64_t columns,
                const std::int32_t level,
                const TerrainClass terrain)
            {
                const auto hash = std::rotl(
                    positionHash(latticeColumn, latticeRow),
                    static_cast<int>((layerAt * 13) % 64));
                const auto west =
                    latticeColumn > 0 ? decor[at - 1] : -1;
                const auto north =
                    latticeRow > 0
                        ? decor
                              [at
                               - static_cast<std::size_t>(columns)]
                        : -1;

                const auto forced =
                    (west >= 0
                     && layer.sprites[static_cast<std::size_t>(west)]
                                .sockets[kEast]
                            != tileset::kOpenSocket)
                    || (north >= 0
                        && layer
                                   .sprites[static_cast<std::size_t>(
                                       north)]
                                   .sockets[kSouth]
                               != tileset::kOpenSocket);

                if (!forced && (hash >> 32) % 256 >= layer.density)
                {
                    return;
                }

                const auto baseId =
                    set.layers[0]
                        .sprites[static_cast<std::size_t>(chosen[at])]
                        .id;
                const auto eastHasBase =
                    latticeColumn + 1 < columns && chosen[at + 1] >= 0;
                const auto southHasBase =
                    at + static_cast<std::size_t>(columns)
                        < chosen.size()
                    && chosen[at + static_cast<std::size_t>(columns)]
                           >= 0;

                std::vector<std::size_t> candidates{};

                for (std::size_t sprite = 0;
                     sprite < layer.sprites.size();
                     ++sprite)
                {
                    if (decorFits(
                            layer.sprites,
                            sprite,
                            baseId,
                            west,
                            north,
                            eastHasBase,
                            southHasBase))
                    {
                        candidates.push_back(sprite);
                    }
                }

                if (candidates.empty())
                {
                    return;
                }

                const auto pick =
                    weightedPick(layer.sprites, candidates, hash);

                decor[at] = static_cast<std::int32_t>(pick);
                plan.push_back(TileDraw{
                    .terrain = terrain,
                    .kind = DrawKind::Sprite,
                    .atlasRow = static_cast<std::uint16_t>(
                        offsets[layerAt] + pick),
                    .frame = frameOf(layer.sprites[pick], clock),
                    .screen = latticeScreen(
                        latticeColumn, latticeRow, level)});
            }

            [[nodiscard]] static bool decorFits(
                const std::vector<Sprite> &sprites,
                const std::size_t candidate,
                const tileset::SpriteId baseId,
                const std::int32_t west,
                const std::int32_t north,
                const bool eastHasBase,
                const bool southHasBase)
            {
                const auto &sprite = sprites[candidate];

                if (std::ranges::find(sprite.on, baseId)
                    == sprite.on.end())
                {
                    return false;
                }

                const auto wanted =
                    west >= 0
                        ? sprites[static_cast<std::size_t>(west)]
                              .sockets[kEast]
                        : tileset::kOpenSocket;

                if (sprite.sockets[kWest] != wanted)
                {
                    return false;
                }

                const auto above =
                    north >= 0
                        ? sprites[static_cast<std::size_t>(north)]
                              .sockets[kSouth]
                        : tileset::kOpenSocket;

                if (sprite.sockets[kNorth] != above)
                {
                    return false;
                }

                if (!eastHasBase
                    && sprite.sockets[kEast] != tileset::kOpenSocket)
                {
                    return false;
                }

                return southHasBase
                       || sprite.sockets[kSouth]
                              == tileset::kOpenSocket;
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

            [[nodiscard]] bool southHasSlab(
                const GridCell cell, const std::int32_t level) const
            {
                const auto south = GridCell{
                    .column = cell.column, .row = cell.row + 1};

                if (south.row >= map.rows())
                {
                    return level <= 0;
                }

                return map.at(south).slabAt(level) != nullptr;
            }

            void addFace(const GridCell cell, const std::int32_t level)
            {
                const auto &held = map.at(cell);

                if (held.slabAt(level) == nullptr)
                {
                    return;
                }

                if (hidden[indexOf(map, cell)]
                    && level > playerHeight)
                {
                    return;
                }

                if (southHasSlab(cell, level))
                {
                    return;
                }

                const auto kind = held.slabAt(level + 1) == nullptr
                                      ? DrawKind::WallRim
                                      : DrawKind::WallBand;

                const auto y =
                    static_cast<std::int32_t>(cell.row + 1) * kUnit
                    - level * kLevelRise;

                const auto x =
                    static_cast<std::int32_t>(cell.column) * kUnit;

                plan.push_back(TileDraw{
                    .terrain = TerrainClass::Cliff,
                    .kind = kind,
                    .screen = {.x = x, .y = y}});
                plan.push_back(TileDraw{
                    .terrain = TerrainClass::Cliff,
                    .kind = kind,
                    .screen = {.x = x + kHalfTile, .y = y}});
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
                const auto *slab = map.at(cell).slabAt(level);

                if (slab == nullptr
                    || slab->overlay != Overlay::Bridge)
                {
                    return;
                }

                if (cutAway(cell, level))
                {
                    return;
                }

                pushQuad(
                    DrawKind::BridgeDeck, slab->terrain, cell, level);
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
                for (const auto &slab : map.at(cell).slabs())
                {
                    if (slab.light >= kShadeBelow)
                    {
                        continue;
                    }

                    if (cutAway(cell, slab.level))
                    {
                        continue;
                    }

                    pushQuad(
                        DrawKind::Shade,
                        slab.terrain,
                        cell,
                        slab.level);

                    if (slab.light < kDenseShadeBelow)
                    {
                        pushQuad(
                            DrawKind::Shade,
                            slab.terrain,
                            cell,
                            slab.level);
                    }
                }
            }

            void pushQuad(
                const DrawKind kind,
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
                        .kind = kind,
                        .screen = {
                            .x = left + part % 2 * kHalfTile,
                            .y = top + part / 2 * kHalfTile}});
                }
            }

            const TileMap &map;
            std::int32_t playerHeight;
            std::uint32_t clock;
            const TilesetBindings &bindings;
            std::vector<bool> hidden;
            std::array<
                tileset::AtlasIndex,
                enums::kCount<TerrainClass>>
                atlas{};
            DrawPlan plan{};
        };
    }

    DrawPlan buildDrawPlan(
        const TileMap &map,
        const GridCell player,
        const std::int32_t playerHeight,
        const std::uint32_t clock,
        const TilesetBindings &bindings)
    {
        return PlanBuilder(
                   map, player, playerHeight, clock, bindings)
            .build();
    }

}
